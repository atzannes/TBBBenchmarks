#!/bin/bash

### Uncomment next line to compile on T2 machine
#CCOPTS=" -m64 -mcpu=v9 -MMD "

DIRS="20k 30k 4M 40M"

for d in ${DIRS}; do
    cd ${d}
    echo "changing into directory ${d}"
    for f in `ls *.gz`; do
        newf=`basename ${f} .gz`
        echo "gunzip -c ${f} > ${newf}"
        gunzip -c ${f} > ${newf}
    done
    cd ../
done
