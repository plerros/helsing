#/bin/bash

if [ $# -ne 2 ]; then
	>&2 echo "Find the greatest common prefix between to strings"
	>&2 echo "Usage: $selfname [str] [prefix]"
	exit
fi

str="$1"
prefix="$2"

for (( i=0; i<${#prefix}; i++ )); do
	if [ "${prefix:$i:1}" != "${str:$i:1}" ]; then
		echo "${str:$i:${#str}}"
		exit
	fi
done

echo "${str:${#prefix}:${#str}}"