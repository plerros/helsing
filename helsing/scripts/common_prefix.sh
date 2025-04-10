#/bin/bash

if [ $# -ne 2 ]; then
	>&2 echo "Find the greatest common prefix between to strings"
	>&2 echo "Usage: $selfname [str1] [str2]"
	exit
fi

shorter="$1"
longer="$2"

if [ ${#1} -gt ${#2} ]; then
	shorter="$2"
	longer="$1"
fi

for (( i=0; i<${#shorter}; i++ )); do
	a="${shorter:$i:1}"
	b="${longer:$i:1}"
	if [ "$a" != "$b" ]; then
		echo -n "\n"
		exit
	fi

	echo -n "$a"
done