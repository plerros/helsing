#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

selfdir="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

n_max=16

b_seq=$(seq 2 2)                # numeral base
pm_seq=$(seq 0 4)               # partition method
m_seq=$(seq 1 $(( n_max / 2 ))) # multiplicand
p_seq=$(seq 2 $n_max)           # product
n_seq=$(seq 2 2 $n_max)         # [n]

# We can't do arrays of structs :(
l_base=()	# numeral base
l_meth=()	# method
l_mult=()	# multiplicand
l_prod=()	# product

len=0
for base in $b_seq; do
	for partition_method in $pm_seq; do
		for multiplicand in $m_seq; do
			for product in $p_seq; do
				l_base[$len]="$base"
				l_meth[$len]="$partition_method"
				l_mult[$len]="$multiplicand"
				l_prod[$len]="$product"

				let "len++"
			done

		done
	done
done
let "len--"

echo "base, method, multiplicand, product, [n]"
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

rc=0
for i in $(seq 0 $len); do
	"$selfdir/../../scripts/configuration/set_cache.sh" "${l_base[$i]}" "${l_meth[$i]}" "${l_mult[$i]}" "${l_prod[$i]}"
	make -j4 OPTIMIZE=-O0 > /dev/null
	for n in $n_seq; do
		if (( $rc == 0 )); then
			echo -e '\e[1A\e[K'"${l_base[$i]} ${l_meth[$i]} ${l_mult[$i]} ${l_prod[$i]}\t$n"
		else
			echo -e "${l_base[$i]} ${l_meth[$i]} ${l_mult[$i]} ${l_prod[$i]}\t$n"
		fi
		./helsing -n "$n" > /dev/null 2>&1
		rc=$?
	done
done

mv configuration.backup configuration.h
