#!/bin/sh

set -eu

cd "$(dirname "$0")"

for std in c++11 c++20 c99 c11; do

	"./compile-${std}.sh"

	for file in $(find .. -type f -size -32760c); do
		if ! ./deflate $file | ./inflate > tmp; then
			echo "inflate error at $file"
			./deflate $file | ./inflate > tmp
		fi
		diff $file tmp
	done

done

rm -f tmp
