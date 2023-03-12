#!/bin/sh

set -eu

cd "$(dirname "$0")"

run_tests() {
	for file in $(find .. -type f -size -32760c -not -name tmp); do
                if ! ./deflate $file -1 $1 | ./inflate $file > tmp; then
			echo "inflate error at $file"
                        ./deflate $file -1 $1 | ./inflate $file > tmp
		fi
		diff $file tmp
	done
}

for std in c++11 c++20 c99 c11; do

	"./compile-${std}.sh"
	run_tests 0

	"./compile-${std}.sh" -DDEFLATE_CHECKSUM
	run_tests 0

	"./compile-${std}.sh" -DDEFLATE_WITH_LUT
	run_tests 0

	"./compile-${std}.sh" -DDEFLATE_CHECKSUM -DDEFLATE_WITH_LUT
	run_tests 0

	"./compile-${std}.sh" -DDEFLATE_CHECKSUM -DDEFLATE_WITH_LUT
	run_tests 1


done

rm -f tmp
