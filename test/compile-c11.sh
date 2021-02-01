#!/bin/sh

exec gcc -std=c11 -O2 -Wall -Wextra -pedantic -I../src "$@" -o inflate inflate-app.c ../src/inflate.c
