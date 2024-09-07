#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2024 Pierro Zachareas
'

selfname=$(basename "$0")

base_min="2"
base_max="12"
part_max="3"
time_min="0.5"

case $# in
	"1")
		out_folder="$1"
		;;

	"2")
		time_min="$1"
		out_folder="$2"
		;;
	"3")
		part_max="$1"
		time_min="$2"
		out_folder="$3"
		;;
	"5")
		base_min="$1"
		base_max="$2"
		part_max="$3"
		time_min="$4"
		out_folder="$5"
		;;
	*)
		echo "For each base find optimal settings and write them to a CSV"
		echo "Usage: $selfname [BASE_MIN] [BASE_MAX] [PARTS_MAX] [TIME_MIN] [OUTPUT_FOLDER]"
		echo -e "The first 2-4 parameters are optional"
		echo -e "BASE_MIN / MAX               numeral base range"
		echo -e "PARTS_MAX                    max partitions"
		echo -e "TIME_MIN                     default config minimum runtime,"
		echo -e "                             increase to narrow down results"
		echo
		echo "Reccomended parameters:"
		echo -e "\tquick(?~?h)  $selfname 2 12 3  0.5"
		echo -e "\tnarrow(?~?h) $selfname 2 12 3  10.0"
		echo -e "\twide(7~24h)  $selfname 2 12 10 0.5"
		exit
		;;
esac

echo "base=[$base_min, $base_max], parts=[1, $part_max], time_min=$time_min, out=$out_folder"

mkdir -p "$out_folder"
b_seq=$(seq $base_min $base_max)        # numeral base

function collect_data () {
	n_min=$1
	n_max=$2
	out_size=$3
	out_file=$4

	pm_seq=$(seq 0 4)        # partition method
	m_seq=$(seq 1 $out_size) # multiplicand
	p_seq=$(seq 2 $out_size) # product

	# We can't do arrays of structs :(
	l_meth=()	# method
	l_mult=()	# multiplicand
	l_prod=()	# product
	l_skip=()	# skip this one
	l_time=()	# runtime
	l_sdev=()	# standard deviation

	len=0
	for partition_method in $pm_seq; do
		for multiplicand in $m_seq; do
			for product in $p_seq; do
				l_meth[$len]="$partition_method"
				l_mult[$len]="$multiplicand"
				l_prod[$len]="$product"
				l_skip[$len]=0

				l_time="0.0"
				l_sdev="0.0"
				let "len++"
			done
		done
	done
	let "len--"

	echo "base\tn\tmethod\tmultiplicand\tproduct"
	for n in $(seq $n_min 2 $n_max); do
		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
				continue
			fi
			# No need to evaluate configurations where partitions requested are more than what's possible.
			if (( ${l_mult[$i]} > $out_size )); then
				continue
			fi
			if (( ${l_prod[$i]} > $out_size )); then
				continue
			fi

			sed -i -e "s|BASE[[:space:]]\+[[:digit:]]\+|BASE $base|g"                                               configuration.h
			sed -i -e "s|PARTITION_METHOD[[:space:]]\+[[:digit:]]\+|PARTITION_METHOD ${l_meth[$i]}|g"               configuration.h
			sed -i -e "s|MULTIPLICAND_PARTITIONS[[:space:]]\+[[:digit:]]\+|MULTIPLICAND_PARTITIONS ${l_mult[$i]}|g" configuration.h
			sed -i -e "s|PRODUCT_PARTITIONS[[:space:]]\+[[:digit:]]\+|PRODUCT_PARTITIONS ${l_prod[$i]}|g"           configuration.h
			make -j4 > /dev/null
			hyperfine --warmup 2 "./helsing -n $n" --export-csv tmp.csv > /dev/null 2>&1
			l_time[$i]=$(awk -F "\"*,\"*" '{print $2}' tmp.csv | awk 'NR>1')
			l_sdev[$i]=$(awk -F "\"*,\"*" '{print $3}' tmp.csv | awk 'NR>1')
			rm tmp.csv

			echo -e "$base\tn$n\t${l_meth[$i]}\t${l_mult[$i]}\t${l_prod[$i]}\t${l_time[$i]}\t${l_sdev[$i]}"

			if [ -z "${l_time[$i]}" ]; then
				l_skip[$i]=1
			fi
		done

		# initialize min
		min=0.0
		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
				continue
			fi
			if (( ${l_mult[$i]} > $out_size )); then
				continue
			fi
			if (( ${l_prod[$i]} > $out_size )); then
				continue
			fi
			min=$(echo "${l_time[$i]} + ${l_sdev[$i]}" | bc)
			break
		done

		# find the min
		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
				continue
			fi
			if (( ${l_mult[$i]} > $out_size )); then
				continue
			fi
			if (( ${l_prod[$i]} > $out_size )); then
				continue
			fi
			if [ 1 -eq "$(echo "${min} > ${l_time[$i]} + ${l_sdev[$i]}" | bc)" ]; then
				min=$(echo "${l_time[$i]} + ${l_sdev[$i]}" | bc)
				#min=${l_time[$i]}
			fi
		done

		# discard slow configurations
		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
				continue
			fi
			if (( ${l_mult[$i]} > $out_size )); then
				continue
			fi
			if (( ${l_prod[$i]} > $out_size )); then
				continue
			fi
			if [ 1 -eq "$(echo "${min} * 2.0 < ${l_time[$i]} - ${l_sdev[$i]}" | bc)" ]; then
				l_skip[$i]=1
			fi
		done
	done
	for i in $(seq 0 $len); do
		if (( ${l_skip[$i]} == 1 )); then
			continue
		fi
		if (( ${l_mult[$i]} > $out_size )); then
			continue
		fi
		if (( ${l_prod[$i]} > $out_size )); then
			continue
		fi
		echo -e "$base\tn$n\t${l_meth[$i]}\t${l_mult[$i]}\t${l_prod[$i]}\t${l_time[$i]}\t${l_sdev[$i]}" >> $out_file
	done

}

cp configuration.h configuration.backup1

sed -i -e "s|ALG_NORMAL[[:space:]]\+true|ALG_NORMAL false|g"       configuration.h
sed -i -e "s|ALG_CACHE[[:space:]]\+false|ALG_CACHE true|g"         configuration.h
sed -i -e "s|SAFETY_CHECKS[[:space:]]\+true|SAFETY_CHECKS false|g" configuration.h

cp configuration.h configuration.backup2

function handle_sigint()
{
	rm -f tmp.csv
	make clean
	mv configuration.backup1 configuration.h
	rm -f configuration.backup2
	exit
}

trap handle_sigint SIGINT

for base in $b_seq; do
	cp configuration.backup2 configuration.h
	sed -i -e "s|BASE[[:space:]]\+[[:digit:]]\+|BASE $base|g" configuration.h
	make -j4 > /dev/null

	n_min=2
	# find n for which runtime is longer than 0.01s
	for i in $(seq $n_min 2 64); do
		n_min=$i

		hyperfine --warmup 2 "./helsing -n $i" --export-csv tmp.csv > /dev/null 2>&1
		runtime=$(awk -F "\"*,\"*" '{print $2}' tmp.csv | awk 'NR>1')
		if [ 1 -eq "$(echo "${runtime} > 0.01" | bc)" ]; then
			break
		fi
	done
	n_max=$n_min
	# find n for which runtime is longer than 0.5s
	for i in $(seq $n_max 2 64); do
		n_max=$i

		hyperfine --warmup 2 "./helsing -n $i" --export-csv tmp.csv > /dev/null 2>&1
		runtime=$(awk -F "\"*,\"*" '{print $2}' tmp.csv | awk 'NR>1')
		if [ 1 -eq "$(echo "${runtime} > ${time_min}" | bc)" ]; then
			break
		fi
	done
	out_file=$(echo "$out_folder/base$base.csv")
	collect_data $n_min $n_max $part_max $out_file
done

mv configuration.backup1 configuration.h
rm configuration.backup2
