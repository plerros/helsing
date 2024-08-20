#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2024 Pierro Zachareas
'

maindir=$(pwd)
dir1="$1"
dir2="$2"

RED='\033[0;31m'
NC='\033[0m' # No Color

if [ $# -eq 0 ]; then
	echo "Compare 2 sets of results. Each should be in it's own folder"
	echo "Usage: compare.sh [path1] [path2]"
	exit
fi

common="$maindir/tmp.common"

for file in "$dir1"/*; do
	cut -f 3-5 "$file" | sort -u > "$common"
	break
done

for file in "$dir1"/*; do
	cut -f 3-5 "$file" | sort -u > tmp
	comm -12 tmp "$common" > tmp2
	rm tmp
	mv tmp2 "$common"
done

for file in "$dir2"/*; do
	cut -f 3-5 "$file" | sort -u > tmp
	comm -12 tmp "$common" > tmp2
	rm tmp
	mv tmp2 "$common"
done

cd "$dir1"
for file in *; do
	cut -f 1-5 "$file" > tmp
	while IFS="" read -r line || [ -n "$p" ]; do
		if [ ! -f "$maindir/$dir2/$file" ]; then
			continue;
		fi

		params=$(echo "$line" | cut -f 3-5)
		res=$(cat "$common" | grep -e "$params")
		if [ -z "$res" ]; then
			continue;
		fi

		res=$(cat "$maindir/$dir2/$file" | grep -e "$line")
		if [ -z "$res" ]; then
			continue;
		fi

		mean[0]=$(cat "$maindir/$dir1/$file" | grep -e "$line" | cut -f 6 )
		mean[1]=$(cat "$maindir/$dir2/$file" | grep -e "$line" | cut -f 6 )

		stddev[0]=$(cat "$maindir/$dir1/$file" | grep -e "$line" | cut -f 7 )
		stddev[1]=$(cat "$maindir/$dir2/$file" | grep -e "$line" | cut -f 7 )

		#echo ${stddev[0]} \/ ${mean[0]}
		if [ 1 -eq "$(echo "${stddev[0]} / ${mean[0]} > 0.03" | bc -l)" ]; then
			echo -e "$line\t${RED}TOO MUCH VARIANCE${NC}"
			continue
		fi
		if [ 1 -eq "$(echo "${stddev[1]} / ${mean[1]} > 0.03" | bc -l)" ]; then
			echo -e "$line\t${RED}TOO MUCH VARIANCE${NC}"
			continue
		fi


		ratio=$( echo "${mean[1]} / ${mean[0]}" | bc -l)

		if [ 1 -eq "$(echo "($ratio < 1.01) && ($ratio > 0.99)" | bc -l)" ]; then
			continue
		fi

		if [ 1 -eq "$(echo "define abs(n) {if ( n > 0 ) return (n);{return (-n);}}; abs(${mean[0]} - ${mean[1]}) > ${stddev[0]} + ${stddev[1]}" | bc -l)" ]; then
			echo -e "$line\t$ratio"
		fi
	done < tmp
	unset IFS
	rm tmp
done

rm "$common"
