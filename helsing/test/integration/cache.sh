#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

selfdir="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

n_max=32

b_seq=$(seq 12 -2 2)    # numeral base
n_seq=$(seq 2 2 $n_max) # [n]

# We can't do arrays of structs :(
l_base=()	# numeral base

len=0
for base in $b_seq; do
	l_base[$len]="$base"
	let "len++"
done
let "len--"

echo -e "base\t[n]"
echo

cp configuration.h configuration.backup
"$selfdir/../../scripts/configuration/set_cache.sh"

function handle_sigint()
{
	make clean
	mv configuration.backup configuration.h
	exit
}

trap handle_sigint SIGINT

for i in $(seq 0 $len); do
	"$selfdir/../../scripts/configuration/set.sh" BASE "${l_base[$i]}"
	make -j4 OPTIMIZE=-O2 > /dev/null
	rc=0
	for n in $n_seq; do
		./helsing -n "$n" > /dev/null 2>&1
		rc=$?
		if (( $rc == 0 )); then
			echo -e '\e[1A\e[K'"${l_base[$i]}\t$n"
		else
			echo -e "${l_base[$i]}\t$n"
			break
		fi
	done
done

mv configuration.backup configuration.h
