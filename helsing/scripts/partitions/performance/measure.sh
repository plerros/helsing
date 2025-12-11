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
		>&2 echo "For each base find optimal settings and write them to a CSV"
		>&2 echo "Usage: $selfname [PARAMETERS_CSV] [OUTPUT_FOLDER]"
		>&2 echo -e "PARAMETERS_CSV               use generate_parameters.sh"
		exit
		;;
esac

"$selfdir/../../system_info.sh"

mkdir -p "$out_folder"

function collect_data () {
	base=$1
	upper_bound=$2
	part_max=$3
	out_file=$4

	pm_seq=$(seq 0 4)        # partition method
	m_seq=$(seq 1 $part_max) # multiplicand
	p_seq=$(seq 2 $part_max) # product

	# We can't do arrays of structs :(
	l_meth=()	# method
	l_mult=()	# multiplicand
	l_prod=()	# product
	l_time=()	# runtime
	l_sdev=()	# standard deviation

	len=0
	for partition_method in $pm_seq; do
		for multiplicand in $m_seq; do
			for product in $p_seq; do
				l_meth[$len]="$partition_method"
				l_mult[$len]="$multiplicand"
				l_prod[$len]="$product"

				l_time="0.0"
				l_sdev="0.0"
				let "len++"
			done
		done
	done
	let "len--"

	for i in $(seq 0 $len); do
		"$selfdir/../../configuration/set_cache.sh" "$base" "${l_meth[$i]}" "${l_mult[$i]}" "${l_prod[$i]}"
		make -j4 > /dev/null 2>&1
		hyperfine --warmup 2 "./helsing -l 0 -u $upper_bound" --export-csv tmp.csv > /dev/null 2>&1
		l_time[$i]=$(awk -F "\"*,\"*" '{print $2}' tmp.csv | awk 'NR>1')
		l_sdev[$i]=$(awk -F "\"*,\"*" '{print $3}' tmp.csv | awk 'NR>1')
		rm tmp.csv

		>&2 echo -e "$base\t$upper_bound\t${l_meth[$i]}\t${l_mult[$i]}\t${l_prod[$i]}\t${l_time[$i]}\t${l_sdev[$i]}"
	done
	for i in $(seq 0 $len); do
		if [ -z "${l_time[$i]}" ]; then
			continue
		fi
		echo -e "$base\t$upper_bound\t${l_meth[$i]}\t${l_mult[$i]}\t${l_prod[$i]}\t${l_time[$i]}\t${l_sdev[$i]}" >> $out_file
	done

}

cp configuration.h configuration.backup1
"$selfdir/../../configuration/set_cache.sh"

function handle_sigint()
{
	rm -f tmp.csv
	make clean
	mv configuration.backup1 configuration.h
	exit
}

trap handle_sigint SIGINT

>&2 echo -e "base\tupper_bound\tmethod\tmultiplicand\tproduct\truntime\tstddev"
while IFS=$'\t' read -r base u_min u_max part_max; do
	number='^[[:digit:]]+$'
	if ! [[ $base =~ $number ]] ; then
		continue
	fi

	out_file=$(echo "$out_folder/base$base.csv")
	collect_data $base $u_max $part_max $out_file
done < "$parameters_file"

mv configuration.backup1 configuration.h
