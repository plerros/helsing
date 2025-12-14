#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

selfname="$(basename "$0")"
selfdir="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

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

"$selfdir/../../system_info.sh"
tempdir=$(mktemp -d) && trap 'rm -rf "$tempdir"' EXIT || exit

# Temporary files
configuration_h_backup1="$tempdir/configuration.backup1"
hyperfine_csv="$tempdir/hyperfine.csv"

mkdir -p "$out_folder"
cp configuration.h "$configuration_h_backup1"
"$selfdir/../../configuration/set_cache.sh"

function cleanup()
{
	make clean
	mv $configuration_h_backup1 configuration.h
	rm -f "$hyperfine_csv"
	exit
}

trap cleanup SIGINT

function collect_data () {
	base=$1
	u_min=$2
	u_max=$3
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

	u="0"
	while [ 1 -eq "$(echo $u '<' $u_max | bc)" ]; do
		# Handle u values
		if [ "$u" == "0" ]; then
			# first loop; initialize
			u="$u_min"
		else
			# nth loop; iterate
			u="$(echo $u '*' $base '*' $base | bc)"
		fi
		if [ 1 -eq "$(echo $u '<' $u_max | bc)" ]; then
			# last loop; cap
			u="$u_max"
		fi

		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
				continue
			fi

			"$selfdir/../../configuration/set_cache.sh" "$base" "${l_meth[$i]}" "${l_mult[$i]}" "${l_prod[$i]}"
			make -j4 > /dev/null 2>&1
			hyperfine --warmup 2 "./helsing -l 0 -u $u" --export-csv "$hyperfine_csv" > /dev/null 2>&1
			l_time[$i]=$(awk -F "\"*,\"*" '{print $2}' "$hyperfine_csv" | awk 'NR>1')
			l_sdev[$i]=$(awk -F "\"*,\"*" '{print $3}' "$hyperfine_csv" | awk 'NR>1')
			rm "$hyperfine_csv"

			echo -e "$base\t$u\t${l_meth[$i]}\t${l_mult[$i]}\t${l_prod[$i]}\t${l_time[$i]}\t${l_sdev[$i]}"

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
			min="$(echo ${l_time[$i]} '+' ${l_sdev[$i]} | bc)"
			break
		done

		# find the min
		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
				continue
			fi
			if [ 1 -eq "$(echo $min '>' ${l_time[$i]} '+' ${l_sdev[$i]} | bc)" ]; then
				min="$(echo ${l_time[$i]} '+' ${l_sdev[$i]} | bc)"
			fi
		done

		# discard slow configurations
		for i in $(seq 0 $len); do
			if (( ${l_skip[$i]} == 1 )); then
				continue
			fi
			if [ 1 -eq "$(echo $min '* 2.0 <' ${l_time[$i]} '-' ${l_sdev[$i]} | bc)" ]; then
				l_skip[$i]=1
			fi
		done
	done
	for i in $(seq 0 $len); do
		if (( ${l_skip[$i]} == 1 )); then
			continue
		fi
		echo -e "$base\t$u\t${l_meth[$i]}\t${l_mult[$i]}\t${l_prod[$i]}\t${l_time[$i]}\t${l_sdev[$i]}" >> $out_file
	done

}

echo -e "base\tupper_bound\tmethod\tmultiplicand\tproduct"
while IFS=$'\t' read -r base u_min u_max part_max; do
	number='^[[:digit:]]+$'
	if ! [[ $base =~ $number ]] ; then
		continue
	fi

	out_file=$(echo "$out_folder/base$base.csv")
	collect_data $base $u_min $u_max $part_max $out_file
done < "$parameters_file"

cleanup
