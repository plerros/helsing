#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

selfname=$(basename "$0")

case $# in
	"2")
		parameters_file="$1"
		out_folder="$2"
		;;
	*)
		echo "For each base find optimal settings and write them to a CSV"
		echo "Usage: $selfname [PARAMETERS_CSV] [OUTPUT_FOLDER]"
		echo -e "PARAMETERS_CSV               use generate_parameters.sh"
		exit
		;;
esac

mkdir -p "$out_folder"

function collect_data () {
	base=$1
	n_min=$2
	n_max=$3
	part_max=$4
	out_file=$5

	pm_seq=$(seq 0 4)        # partition method
	m_seq=$(seq 1 $part_max) # multiplicand
	p_seq=$(seq 2 $part_max) # product

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

	for n in $(seq $n_min 2 $n_max); do
		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
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
			min=$(echo "${l_time[$i]} + ${l_sdev[$i]}" | bc)
			break
		done

		# find the min
		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
				continue
			fi
			if [ 1 -eq "$(echo "${min} > ${l_time[$i]} + ${l_sdev[$i]}" | bc)" ]; then
				min=$(echo "${l_time[$i]} + ${l_sdev[$i]}" | bc)
			fi
		done

		# discard slow configurations
		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
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
		echo -e "$base\tn$n\t${l_meth[$i]}\t${l_mult[$i]}\t${l_prod[$i]}\t${l_time[$i]}\t${l_sdev[$i]}" >> $out_file
	done

}

cp configuration.h configuration.backup1

sed -i -e "s|ALG_NORMAL[[:space:]]\+true|ALG_NORMAL false|g"       configuration.h
sed -i -e "s|ALG_CACHE[[:space:]]\+false|ALG_CACHE true|g"         configuration.h
sed -i -e "s|SAFETY_CHECKS[[:space:]]\+true|SAFETY_CHECKS false|g" configuration.h

function handle_sigint()
{
	rm -f tmp.csv
	make clean
	mv configuration.backup1 configuration.h
	exit
}

trap handle_sigint SIGINT

echo -e "base\tn\tmethod\tmultiplicand\tproduct"
while IFS=$'\t' read -r base n_min n_max part_max; do
	number='^[[:digit:]]+$'
	if ! [[ $base =~ $number ]] ; then
		continue
	fi

	out_file=$(echo "$out_folder/base$base.csv")
	collect_data $base $n_min $n_max $part_max $out_file
done < "$parameters_file"

mv configuration.backup1 configuration.h
