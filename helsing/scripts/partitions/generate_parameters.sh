#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

selfname="$(basename "$0")"
selfdir="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

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

if [ 1 -eq "$(echo "${time_min} < 0.01" | bc)" ]; then
	echo "[TIME_MIN] should be >= 0.01"
	exit
fi

tempdir=$(mktemp -d) && trap 'rm -rf "$tempdir"' EXIT || exit

cp configuration.h configuration.backup1
"$selfdir/../configuration/set_cache.sh"
cp configuration.h configuration.backup2

function handle_sigint()
{
	make clean
	mv configuration.backup1 configuration.h
	rm -f configuration.backup2
	exit
}

trap handle_sigint SIGINT

b_seq=$(seq $base_min $base_max)        # numeral base

function get_runtime()
{
	hyperfine --warmup 2 "./helsing -l 0 -u $2" --export-csv "$1/tmp.csv" > /dev/null 2>&1
	awk -F "\"*,\"*" '{print $2}' "$1/tmp.csv"  | awk 'NR>1'
}

echo -e "base\tn\truntime"
echo -e "base\tn_min\tn_max\tpart_max" > "$out_file"

for base in $b_seq; do
	cp configuration.backup2 configuration.h
	"$selfdir/../configuration/set.sh" BASE "$base"
	make -j4 > /dev/null

	ub_min="1"
	ub_max="1"

	# Loop 1: Generate the initial number range [a, b] for the upper bound.
	while true; do
		runtime=$(get_runtime $tempdir $ub_max)
		
		if [ -z "${runtime}" ]; then
			continue
		fi

		if [ 1 -eq "$(echo "${runtime} > ${time_min}" | bc)" ]; then
			break;
		fi

		ub_min="$ub_max"
		multiplier="$(echo "${time_min} * 1.5 / ${runtime}" | bc)"
		if [ 1 -eq "$(echo "${multiplier} < 10" | bc)" ]; then
			multiplier="10"
		fi

		ub_max="$(echo "${ub_max} * ${multiplier}" | bc)"
	done

	# Loop 2: Divide iteratively to find a good enough upper bound
	while true; do
		center="$(echo "(${ub_min} + ${ub_max}) / 2" | bc)"

		runtime=$(get_runtime $tempdir $center)

		if [ 1 -eq "$(echo "${runtime} < ${time_min}" | bc)" ]; then
			ub_min="$center"
		elif [ 1 -eq "$(echo "${runtime} > ${time_min} * 2" | bc)" ]; then
			ub_max="$center"
		else
			echo -e "$base\t$center\t$part_max" >> "$out_file"
			echo -e "$base\t$center\t$part_max"
			break;
		fi
	done
done

mv configuration.backup1 configuration.h
rm configuration.backup2