#!/bin/bash

### Uncomment next line to compile on T2 machine
#CCOPTS=" -m64 -mcpu=v9 -MMD "

DIRS="gSparse gMed gDense road_usa usroads"

for d in ${DIRS}; do
    cd ${d}
    echo "changing into directory ${d}"
    for f in `ls *.gz`; do
        echo "Gun-zipping '${f}'"
        newf=`basename ${f} .gz`
        echo "gunzip ${f} -c > ${newf}"
        gunzip ${f} -c > ${newf}
    done
    cd ../
done
