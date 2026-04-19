#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

if [ $# -lt 2 ]; then
	exit
fi

if [ ! -d "configuration" ]; then
	>&2 echo "no configuration in current folder"
	exit
fi

# We don't need to check. Running all 3 unconditionally should be fine.
find configuration -type f -name "*.h" -exec sed -i -e "s|$1[[:space:]]\+true|$1 $2|g"          {} \;
find configuration -type f -name "*.h" -exec sed -i -e "s|$1[[:space:]]\+false|$1 $2|g"         {} \;
find configuration -type f -name "*.h" -exec sed -i -e "s|$1[[:space:]]\+[[:digit:]]\+|$1 $2|g" {} \;
