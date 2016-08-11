#!/bin/bash

function compiling {
	echo "$1" | ./nc > tmp.s
	if [ $? -ne 0 ]; then
		echo "Compilation Falied for: $1"
		exit
	fi
	gcc -o tmp.out parser.c tmp.s
	if [ $? -ne 0 ]; then
		echo "GCC Error"
		exit
	fi
}

function testing {
	wanted="$1"
	got="$2"
	compiling "$got"
	result=$(./tmp.out)
	echo "$result"
	if [ "$result" != $wanted ]; then
		echo "Testing failed. $wanted was expected but I got $got"
		exit
	fi
}

function uncertain_tests {
	arg1="$1"
	echo "$arg1" | ./nc > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "Should Fail, but compiled successfully: $arg1"
		#exit
	else
		echo "Failed to compile symbol: $arg1"
	fi
}

make -s nc
testing 0 0
testing abc '"abc"'
uncertain_tests '"abc"'
rm -f tmp.out tmp.s
echo "testing done. success"