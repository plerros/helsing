#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2024 Pierro Zachareas
'

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

function handle_sigint()
{
	make clean
	mv configuration.backup configuration.h
	exit
}

trap handle_sigint SIGINT

sed -i -e "s|ALG_NORMAL[[:space:]]\+false|ALG_NORMAL true|g"       configuration.h
sed -i -e "s|ALG_CACHE[[:space:]]\+false|ALG_CACHE true|g"         configuration.h
sed -i -e "s|SAFETY_CHECKS[[:space:]]\+false|SAFETY_CHECKS true|g" configuration.h

rc=0
for i in $(seq 0 $len); do
	sed -i -e "s|BASE[[:space:]]\+[[:digit:]]\+|BASE ${l_base[$i]}|g"                                       configuration.h
	sed -i -e "s|PARTITION_METHOD[[:space:]]\+[[:digit:]]\+|PARTITION_METHOD ${l_meth[$i]}|g"               configuration.h
	sed -i -e "s|MULTIPLICAND_PARTITIONS[[:space:]]\+[[:digit:]]\+|MULTIPLICAND_PARTITIONS ${l_mult[$i]}|g" configuration.h
	sed -i -e "s|PRODUCT_PARTITIONS[[:space:]]\+[[:digit:]]\+|PRODUCT_PARTITIONS ${l_prod[$i]}|g"           configuration.h
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
