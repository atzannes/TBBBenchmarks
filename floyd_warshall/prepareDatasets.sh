#!/bin/bash

### Uncomment the next line to compile on T2 machine
# CCOPTS=" -m64 -mcpu=v9 -MMD "

DIRS="64 256 512 1024 2048"

for d in ${DIRS}; do
    cd ${d}
    echo "changing into directory ${d}"
    for f in `ls *.gz`; do
        echo "Gun-zipping '${f}'"
        newf=`basename ${f} .gz`
        echo "gunzip -c ${f} > ${newf}"
        gunzip -c ${f} > ${newf}
    done
    for f in `ls *.c`; do
        echo "Compiling '${f}'"
        gcc -c ${CCOPTS} ${f}
    done
    cd ../
done
