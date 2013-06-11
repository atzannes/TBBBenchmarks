#!/bin/bash
# runs the configurations to be used for paper submission
DIRS="floyd_warshall SpMV queens tsp matmult convolution bfs"

for d in ${DIRS}; do
    cd ${d}
    echo "changing into directory ${d}"
    benchmarker -runConf ser -runConf par -postProcess cycletable -nX 10 $@
    cd ../
done

