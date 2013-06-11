#!/bin/bash

### Uncomment next line to compile on T2 machine
#CCOPTS=" -m64 -mcpu=v9 -MMD "

DIRS="1024 2048 4096 128 64"

for d in ${DIRS}; do
    cd ${d}
    echo "changing into directory ${d}"
    for f in `ls *.gz`; do
        echo "Gun-zipping '${f}'"
        newf=`basename ${f} .gz`
        echo "gunzip ${f} -c > ${newf}"
        gunzip ${f} -c > ${newf}
    done
    for f in `ls *.c`; do
        echo "Compiling '${f}'"
        gcc -c ${CCOPTS} ${f}
    done
    cd ../
done
