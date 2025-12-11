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
time="0.5"

case $# in
	"5")
		base_min="$1"
		base_max="$2"
		part_max="$3"
		time="$4"
		out_file="$5"
		;;
	*)
		echo "Generate csv with parameters for find.sh/measure.sh"
		echo "Usage: $selfname [BASE_MIN] [BASE_MAX] [PARTS_MAX] [TIME] [OUT_CSV]"
		echo -e "BASE_MIN / MAX               numeral base range"
		echo -e "PARTS_MAX                    max partitions"
		echo -e "TIME                         default config target runtime,"
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

if [ 1 -eq "$(echo $time '< 0.1' | bc)" ]; then
	echo "[TIME] should be >= 0.1"
	exit
fi

"$selfdir/../system_info.sh"
tempdir=$(mktemp -d) && trap 'rm -rf "$tempdir"' EXIT || exit

# Temporary files
checkpoint="a.checkpoint"
configuration_h_backup1="$tempdir/configuration.backup1"
configuration_h_backup2="$tempdir/configuration.backup2"

cp configuration.h $configuration_h_backup1
"$selfdir/../configuration/set_cache.sh"
cp configuration.h $configuration_h_backup2

function cleanup()
{
	make clean
	mv $configuration_h_backup1 configuration.h
	rm -f "a.checkpoint $configuration_h_backup2 $checkpoint"
	exit
}

trap cleanup SIGINT

b_seq=$(seq $base_min $base_max)        # numeral base

echo -e "base\tu_min\tu_max\tpart_max"
echo -e "base\tu_min\tu_max\tpart_max" > "$out_file"

for base in $b_seq; do
	cp "$configuration_h_backup2" configuration.h
	"$selfdir/../configuration/set.sh" BASE "$base"
	make -j4 > /dev/null 2>&1

	upper_bound="1844674407370955"
	task_size="99999999"
	time_u_min="0.1"
	time_u_max="$time"

	# Generate an initial estimate of the upper bound by capturing the last
	# checkpoint within the target runtime
	u_min="$($selfdir/../timeout.sh $time_u_min -l 0 -u $upper_bound -s $task_size)"
	u_max="$($selfdir/../timeout.sh $time_u_max -l 0 -u $upper_bound -s $task_size)"

	# Loop 1: Generate the initial number range [a, b] for the upper bound.
	below="$u_max"
	above="$below"
	while true; do
		runtime=$("$selfdir/../runtime.sh" -l 0 -u "$above")
		if [ -z "$runtime" ]; then
			break
		fi
		if [ 1 -eq "$(echo $runtime '>' $time_u_max | bc -l )" ]; then
			break
		fi

		below="$above"
		multiplier="$(echo $time_u_max '* 1.5 /' $runtime | bc )"
		if [ 1 -eq "$(echo $multiplier '< 2' | bc )" ]; then
			multiplier="2"
		fi

		above="$(echo $above '*' $multiplier | bc )"
	done

	# Loop 2: Divide iteratively to find a good enough upper bound
	while true; do
		center="$(echo '(' $below '+' $above ') / 2' | bc )"
		# Prevent Infinite loops
		if [ "$below" -eq "$center" ]; then
			break
		fi
		if [ "$above" -eq "$center" ]; then
			break
		fi

		runtime=$("$selfdir/../runtime.sh" -l 0 -u "$center")
		if [ -z "${runtime}" ]; then
			break
		fi

		if [ 1 -eq "$(echo $runtime '<' $time_u_max '* 0.9' | bc -l )" ]; then
			below="$center"
		elif [ 1 -eq "$(echo $runtime '>' $time_u_max '* 1.1' | bc -l )" ]; then
			above="$center"
		else
			u_max="$center"
			break
		fi
	done

	echo -e "$base\t$u_min\t$u_max\t$part_max"
	echo -e "$base\t$u_min\t$u_max\t$part_max" >> "$out_file"
done

cleanup
