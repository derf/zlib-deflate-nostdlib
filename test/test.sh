#!/bin/sh

set -eu

cd "$(dirname "$0")"

run_tests() {
	for file in $(find .. -type f -size -32760c); do
		if ! ./deflate $file | ./inflate > tmp; then
			echo "inflate error at $file"
			./deflate $file | ./inflate > tmp
		fi
		diff $file tmp
	done
}

for std in c++11 c++20 c99 c11; do

	"./compile-${std}.sh"
	run_tests

	"./compile-${std}.sh" -DDEFLATE_CHECKSUM
	run_tests

	"./compile-${std}.sh" -DDEFLATE_WITH_LUT
	run_tests

	"./compile-${std}.sh" -DDEFLATE_CHECKSUM -DDEFLATE_WITH_LUT
	run_tests

done

rm -f tmp
