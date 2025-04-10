#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

selfdir="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

"$selfdir/set.sh" ALG_NORMAL    false
"$selfdir/set.sh" ALG_CACHE     true
"$selfdir/set.sh" SAFETY_CHECKS false

if [ $# -eq 4 ]; then
	"$selfdir/set.sh" BASE                    "$1"
	"$selfdir/set.sh" PARTITION_METHOD        "$2"
	"$selfdir/set.sh" MULTIPLICAND_PARTITIONS "$3"
	"$selfdir/set.sh" PRODUCT_PARTITIONS      "$4"
fi
