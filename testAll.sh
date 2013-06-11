#!/bin/bash
# Runs an extensive battery of tests on the machine
benchmarker -runConf TestSer -runConf TestPar -postProcess cycletable $@

#DIRS="floyd_warshall SpMV queens tsp matmult convolution bfs"

#for d in ${DIRS}; do
#    cd ${d}
#    echo "changing into directory ${d}"
#    benchmarker -runConf TestSer -runConf TestPar -postProcess cycletable $@
#    cd ../
#done

