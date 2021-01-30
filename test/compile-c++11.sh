#!/bin/sh

exec g++ -std=c++11 -Wall -Wextra -pedantic -I../src -o inflate inflate-app.c ../src/inflate.c
