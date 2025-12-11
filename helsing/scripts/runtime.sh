#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

tempdir=$(mktemp -d) && trap 'rm -rf "$tempdir"' EXIT || exit
hyperfine_csv="$tempdir/hyperfine.csv"

command="./helsing $@" 
hyperfine "$command" -M 1 --export-csv "$hyperfine_csv" > /dev/null 2>&1
if [ ! -s "$hyperfine_csv" ]; then
	exit -1
fi

awk -F "\"*,\"*" '{print $2}' "$hyperfine_csv"  | awk 'NR>1'
