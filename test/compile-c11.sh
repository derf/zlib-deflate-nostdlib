#!/bin/sh

exec gcc -std=c11 -Wall -Wextra -pedantic -I../src -o inflate inflate-app.c ../src/inflate.c
