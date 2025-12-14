#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

time=$1
shift 1

tempdir=$(mktemp -d) && trap 'rm -rf "$tempdir"' EXIT || exit
checkpoint="$tempdir/a.checkpoint"

timeout "$time" ./helsing -c "$checkpoint" $@ > /dev/null 2>&1
if [ ! -f "$checkpoint" ]; then
	echo "0"
	exit -1
fi

tail -1 "$checkpoint" | grep -o '[[:digit:]]* '