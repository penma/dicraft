#!/bin/sh
if [ ! -e "$1" ]
then
	echo "Failed to build DEF file: $1 doesn't exist" >&2
	exit 1
fi

echo "LIBRARY ${1##*/}"
echo "EXPORTS"
objdump -p "$1" | while read line
do
	if [ "$line" = "[Ordinal/Name Pointer] Table" ]
	then
		while read symline
		do
			symname="${symline##*] }"
			if [ -n "$symname" ]
			then
				echo "$symname DATA"
			else
				exit 0
			fi
		done
	fi
done
