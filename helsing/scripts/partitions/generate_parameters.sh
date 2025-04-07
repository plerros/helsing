#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

selfname=$(basename "$0")

base_min="2"
base_max="12"
part_max="3"
time_min="0.5"

case $# in
	"5")
		base_min="$1"
		base_max="$2"
		part_max="$3"
		time_min="$4"
		out_file="$5"
		;;
	*)
		echo "Generate csv with parameters for find.sh/measure.sh"
		echo "Usage: $selfname [BASE_MIN] [BASE_MAX] [PARTS_MAX] [TIME_MIN] [OUT_CSV]"
		echo -e "BASE_MIN / MAX               numeral base range"
		echo -e "PARTS_MAX                    max partitions"
		echo -e "TIME_MIN                     default config minimum runtime,"
		echo -e "                             increase to narrow down results"
		echo -e "OUT_CSV                      output file"
		echo
		echo "Recommended parameters:"
		echo -e "\tquick(?~?h)  $selfname 2 12 3  0.5  [OUT_CSV]"
		echo -e "\tnarrow(?~?h) $selfname 2 12 3  10.0 [OUT_CSV]"
		echo -e "\twide(7~24h)  $selfname 2 12 10 0.5  [OUT_CSV]"
		exit
		;;
esac


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

b_seq=$(seq $base_min $base_max)        # numeral base

echo -e "base\tn\truntime"
echo -e "base\tn_min\tn_max\tpart_max" > "$out_file"

for base in $b_seq; do
	cp configuration.backup2 configuration.h
	sed -i -e "s|BASE[[:space:]]\+[[:digit:]]\+|BASE $base|g" configuration.h
	make -j4 > /dev/null

	# find n for which runtime is longer than 0.01s
	n_min=2
	for i in $(seq $n_min 2 64); do

		hyperfine --warmup 2 "./helsing -n $i" --export-csv tmp.csv > /dev/null 2>&1
		runtime=$(awk -F "\"*,\"*" '{print $2}' tmp.csv | awk 'NR>1')
		echo -e "$base\t$i\t$runtime"

		if [ -z "${runtime}" ]; then
			continue
		fi

		n_min=$i
		if [ 1 -eq "$(echo "${runtime} > 0.01" | bc)" ]; then
			break
		fi
	done

	# find n for which runtime is longer than [time_min] seconds
	n_max=$n_min
	for i in $(seq $n_max 2 64); do
		hyperfine --warmup 2 "./helsing -n $i" --export-csv tmp.csv > /dev/null 2>&1
		runtime=$(awk -F "\"*,\"*" '{print $2}' tmp.csv | awk 'NR>1')
		echo -e "$base\t$i\t$runtime"

		if [ -z "${runtime}" ]; then
			continue
		fi

		n_max=$i
		if [ 1 -eq "$(echo "${runtime} > ${time_min}" | bc)" ]; then
			echo -e "$base\t$n_min\t$n_max\t$part_max" >> "$out_file"
			break
		fi
	done
done

mv configuration.backup1 configuration.h
rm configuration.backup2
rm tmp.csv