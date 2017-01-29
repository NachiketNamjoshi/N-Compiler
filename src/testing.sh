#!/bin/bash

function compile {
  echo "$1" | ./nc > tmp.s
  if [ $? -ne 0 ]; then
    echo "Failed to compile $1"
    exit
  fi
  gcc -o tmp.out parser.c tmp.s
  if [ $? -ne 0 ]; then
    echo "GCC failed"
    exit
  fi
}

function assertequal {
  if [ "$1" != "$2" ]; then
    echo "Test failed: $2 expected but got $1"
    exit
  fi
}

function testast {
  result="$(echo "$2" | ./nc -a)"
  if [ $? -ne 0 ]; then
    echo "Failed to compile $1"
    exit
  fi
  assertequal "$result" "$1"
}

function test {
  compile "$2"
  assertequal "$(./tmp.out)" "$1"
}

function testfail {
  expr="$1"
  echo "$expr" | ./nc > /dev/null 2>&1
  if [ $? -eq 0 ]; then
    echo "Should fail to compile, but succeded: $expr"
    exit
  fi
}

make -s nc

testast '1' '1;'
testast '(+ (- (+ 1 2) 3) 4)' '1+2-3+4;'
testast '(+ (+ 1 (* 2 3)) 4)' '1+2*3+4;'
testast '(+ (* 1 2) (* 3 4))' '1*2+3*4;'
testast '(+ (/ 4 2) (/ 6 3))' '4/2+6/3;'
testast '(/ (/ 24 2) 4)' '24/2/4;'

testast '(= a 3)' 'a=3;'
testast 'a()' 'a();'
testast 'a(b,c,d,e,f,g)' 'a(b,c,d,e,f,g);'
testast '"abc"' '"abc";'

test 0 '0;'

test 3 '1+2;'
test 3 '1 + 2;'
test 10 '1+2+3+4;'
test 11 '1+2*3+4;'
test 14 '1*2+3*4;'
test 4 '4/2+6/3;'
test 3 '24/2/4;'

test 2 '1;2;'
test 3 'a=1;a+2;'
test 102 'a=1;b=48+2;c=a+b;c*2;'

test 25 'sum2(20, 5);'
test 15 'sum5(1, 2, 3, 4, 5);'
test a3 'printf("a");3;'
test abc5 'printf("%s", "abc");5;'

testfail '0abc;'
testfail '1+;'

echo "All tests passed"
