#!/bin/bash

function testing {
	wanted="$1"
	got="$2"

	echo "$got" | ./nc > tmp.s
	if [ ! $? ]; then
		echo "Cant compile $got"
		exit
	fi
	gcc -o tmp.out int_parser.c tmp.s || exit
	result=$(./tmp.out)
	echo "$result"
	if [ "$result" != $wanted ]; then
		echo "Testing failed. $wanted was expected but I got $got"
		exit
	fi
}
make nc
testing 0 0
testing 7 7
rm -f tmp.out tmp.s
echo "testing done. success"