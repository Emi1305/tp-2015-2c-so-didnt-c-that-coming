#!/bin/bash
gcc -I"/home/utnso/git/commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/memoria.d" -MT"src/memoria.d" -o "src/memoria.o" "../src/memoria.c"

gcc -L"/home/utnso/git/commons/Debug" -o "memoria"  ./src/memoria.o   -lpthread -lcommons
