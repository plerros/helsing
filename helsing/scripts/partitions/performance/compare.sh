#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

selfname="$(basename "$0")"
selfdir="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

if [ $# -ne 2 ]; then
	echo "Compare 2 sets of results. Each should be in it's own folder"
	echo "Usage: $selfname [path1] [path2]"
	exit
fi

# Generate output csv filename based on input paths
path1="$1"
path2="$2"
common_path_prefix="$("$selfdir/../../common_prefix.sh" "$path1" "$path2")"
common_path_prefix="$(dirname "$common_path_prefix")"
id1="$("$selfdir/../../subtract_prefix.sh" "$path1" "$common_path_prefix/")"
id2="$("$selfdir/../../subtract_prefix.sh" "$path2" "$common_path_prefix/")"
id1="${id1///}"
id2="${id2///}"
if [ -z "$id1" ]; then
	>&2 echo "Is $path1 a subpath of $path2?"
	exit
fi
if [ -z "$id2" ]; then
	>&2 echo "Is $path2 a subpath of $path1?"
	exit
fi
csvout=$(echo "$id1 vs $id2.csv")
if [ -f "$csvout" ]; then
	echo "$csvout already exists!"
	exit
fi

echo -e "base\tn\tmethod\tmultiplicand\tproduct\tmean1\tstddev1\tmean2\tstddev2" > "$csvout"

RED='\033[0;31m'
NC='\033[0m'     # No Color

tempdir=$(mktemp -d) && trap 'rm -rf "$tempdir"' EXIT || exit
find "$path1" -type f -name "base*.csv" > "$tempdir/path1_bases"


while read -r file; do

	cut -f 1-5 "$file" > "$tempdir/tmp1"
	name=$(basename "$file")

	echo "$path2"
	echo "$name"
	if [ ! -f "$path2/$name" ]; then
		continue;
	fi

	while IFS="" read -r line || [ -n "$p" ]; do
		res=$(echo "$line" | cut -f 4 | grep -w "1")
		if [ ! -z "$res" ]; then
			continue
		fi

		mean[0]="0.0"
		stddev[0]="inf"
		variation_coeff[0]="0.0"
		mean[1]="0.0"
		stddev[1]="inf"
		variation_coeff[1]="0.0"

		isin[0]=$(cat "$path1/$name" | grep -e "$line	")
		isin[1]=$(cat "$path2/$name" | grep -e "$line	")
		if [ -z "${isin[0]}" -o  -z "${isin[1]}" ]; then
			continue;
		fi

		if [ ! -z "${isin[0]}" ]; then
			mean[0]=$(cat "$path1/$name" | grep -e "$line	" | cut -f 6 )
			stddev[0]=$(cat "$path1/$name" | grep -e "$line	" | cut -f 7 )
			variation_coeff[0]=$( echo "${stddev[0]} / ${mean[0]}" | bc -l)
		fi
		if [ ! -z "${isin[1]}" ]; then
			mean[1]=$(cat "$path2/$name" | grep -e "$line	" | cut -f 6 )
			stddev[1]=$(cat "$path2/$name" | grep -e "$line	" | cut -f 7 )
			variation_coeff[1]=$( echo "${stddev[1]} / ${mean[1]}" | bc -l)
		fi

		if [ 1 -eq "$(echo "${variation_coeff[0]} > 0.03" | bc -l)" ]; then
			echo -e "$line\t${RED}SKIPPED, VARIATION COEFFICIENT > 3%${NC}" >&2
			continue
		fi
		if [ 1 -eq "$(echo "${variation_coeff[1]} > 0.03" | bc -l)" ]; then
			echo -e "$line\t${RED}SKIPPED, VARIATION COEFFICIENT > 3%${NC}" >&2
			continue
		fi

		echo -e "$line\t${mean[0]}\t${stddev[0]}\t${mean[1]}\t${stddev[1]}" >> "$csvout"
	done < "$tempdir/tmp1"
	unset IFS

done < "$tempdir/path1_bases"

python3 "$selfdir/plot_relative.py" "$csvout"