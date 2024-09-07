#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2024 Pierro Zachareas
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

tmp1="$maindir/tmp1.txt"
tmp2="$maindir/tmp2.txt"

for file in $(find "$dir1" -type f -name "base*.csv"); do
	cut -f 3-5 "$file" | sort -u > "$common"
	break
done

for file in $(find "$dir1" -type f -name "base*.csv"); do
	cut -f 3-5 "$file" | sort -u > "$tmp1"
	comm -12 "$tmp1" "$common" > "$tmp2"
	rm "$tmp1"
	mv "$tmp2" "$common"
done

for file in $(find "$dir2" -type f -name "base*.csv"); do
	cut -f 3-5 "$file" | sort -u > "$tmp1"
	comm -12 "$tmp1" "$common" > "$tmp2"
	rm "$tmp1"
	mv "$tmp2" "$common"
done

echo -e "base\tn\tmethod\tmultiplicand\tproduct\tmean1\tstddev1\tmean2\tstddev2" > "$csvout"

for file in $(find "$dir1" -type f -name "base*.csv"); do
	cut -f 1-5 "$file" > "$tmp1"
	name=$(basename $file)
	while IFS="" read -r line || [ -n "$p" ]; do
		if [ ! -f "$maindir/$dir2/$name" ]; then
			continue;
		fi

		params=$(echo "$line" | cut -f 3-5)
		res=$(cat "$common" | grep -e "$params")
		if [ -z "$res" ]; then
			continue;
		fi

		res=$(cat "$maindir/$dir2/$name" | grep -e "$line	")
		if [ -z "$res" ]; then
			continue;
		fi

		mean[0]=$(cat "$maindir/$dir1/$name" | grep -e "$line	" | cut -f 6 )
		mean[1]=$(cat "$maindir/$dir2/$name" | grep -e "$line	" | cut -f 6 )

		stddev[0]=$(cat "$maindir/$dir1/$name" | grep -e "$line	" | cut -f 7 )
		stddev[1]=$(cat "$maindir/$dir2/$name" | grep -e "$line	" | cut -f 7 )

		variation_coeff[0]=$( echo "${stddev[0]} / ${mean[0]}" | bc -l)
		variation_coeff[1]=$( echo "${stddev[1]} / ${mean[1]}" | bc -l)
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
done

rm "$common"
rm -f "$tmp1" "$tmp2"

python3 "$selfdir/plot_relative.py" "$csvout"