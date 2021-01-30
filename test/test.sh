#!/bin/sh

set -eu

cd "$(dirname "$0")"

./compile.sh

for file in $(find .. -type f -size -32760c); do
	if ! ./deflate $file | ./inflate > tmp; then
		echo "inflate error at $file"
		./deflate $file | ./inflate > tmp
	fi
	diff $file tmp
done

rm -f tmp
