#!/bin/sh

exec gcc -std=c99 -O2 -Wall -Wextra -pedantic -I../src "$@" -o inflate inflate-app.c ../src/inflate.c
