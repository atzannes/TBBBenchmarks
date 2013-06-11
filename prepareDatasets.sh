#!/bin/bash
# central script to unzip all datasets and compile all .c files to .o files
ROOTDIR=`pwd`
DIRS="floyd_warshall SpMV matmult convolution bfs/data"

for d in ${DIRS}; do
    cd ${d}
    echo "changing into directory ${d}"
    ./prepareDatasets.sh
    cd ${ROOTDIR}
done

