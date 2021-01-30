#!/bin/sh

exec gcc -ggdb -Wall -Wextra -pedantic -I../src -o inflate inflate-app.c ../src/inflate.c
