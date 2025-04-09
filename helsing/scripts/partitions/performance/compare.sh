#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

selfname=$(basename "$0")
selfdir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

maindir=$(pwd)
dir1="$1"
dir2="$2"

RED='\033[0;31m'
NC='\033[0m' # No Color

if [ $# -eq 0 ]; then
	echo "Compare 2 sets of results. Each should be in it's own folder"
	echo "Usage: $selfname [path1] [path2]"
	exit
fi

common="$maindir/tmp.common"
csvout=$(echo "$1 vs $2.csv" | sed --expression='s|/||g')

if [ -f "$csvout" ]; then
	echo "$csvout already exists!"
	exit
fi

tmp0="$maindir/tmp0.txt"
tmp1="$maindir/tmp1.txt"
tmp2="$maindir/tmp2.txt"

echo -e "base\tn\tmethod\tmultiplicand\tproduct\tmean1\tstddev1\tmean2\tstddev2" > "$csvout"

find "$dir1" -type f -name "base*.csv" > "$tmp0"

while read -r file; do
	cut -f 1-5 "$file" > "$tmp1"
	name=$(basename "$file")

	echo "$maindir"
	echo "$dir2"
	echo "$name"
	if [ ! -f "$maindir/$dir2/$name" ]; then
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

		isin[0]=$(cat "$maindir/$dir1/$name" | grep -e "$line	")
		isin[1]=$(cat "$maindir/$dir2/$name" | grep -e "$line	")
		if [ -z "${isin[0]}" -o  -z "${isin[1]}" ]; then
			continue;
		fi

		if [ ! -z "${isin[0]}" ]; then
			mean[0]=$(cat "$maindir/$dir1/$name" | grep -e "$line	" | cut -f 6 )
			stddev[0]=$(cat "$maindir/$dir1/$name" | grep -e "$line	" | cut -f 7 )
			variation_coeff[0]=$( echo "${stddev[0]} / ${mean[0]}" | bc -l)
		fi
		if [ ! -z "${isin[1]}" ]; then
			mean[1]=$(cat "$maindir/$dir2/$name" | grep -e "$line	" | cut -f 6 )
			stddev[1]=$(cat "$maindir/$dir2/$name" | grep -e "$line	" | cut -f 7 )
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
	done < "$tmp1"
	unset IFS
	rm "$tmp1"
done < "$tmp0"

rm -f "$common" "$tmp0" "$tmp1" "$tmp2"

python3 "$selfdir/plot_relative.py" "$csvout"