#!/bin/sh

# g++ as provided by Debian Buster (used for CI tests) does not support c++20
exec g++ -std=c++2a -O2 -Wall -Wextra -pedantic -I../src "$@" -o inflate inflate-app.c ../src/inflate.c
