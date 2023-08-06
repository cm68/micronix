#!/bin/bash
rm cgetf.*

valgrind --leak-check=full pp/pp -i include/ -d unix -x -o cgetf.i lib/libws/cgetf.c
valgrind --leak-check=full p1/p1 -o cgetf.1 cgetf.i
valgrind --leak-check=full p2/p2 -o cgetf.s cgetf.1
