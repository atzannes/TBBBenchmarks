#!/bin/bash

DIRS="bfs convolution floyd_warshall matmult queens tsp SpMV fib uts-1.1"
HOSTNAME=`hostname`
TARFILE=${HOSTNAME}-results-git-repos.tar
GZIPFILE=${TARFILE}.gz

echo ${HOSTNAME}

rm -f ${TARFILE} ${GZIPFILE}

# Make the tar file

for dir in ${DIRS}
do
    echo "Chekcing directory '${dir}'"
    if [ -d "${dir}/results" ]; then
        echo "Adding ${dir}/results to ${TARFILE}"
        tar uvf ${TARFILE} ${dir}/results
    else
        echo "'${dir}/results' is not a directory :("
    fi

    if [ -d "${dir}/Stats" ]; then
        echo "Adding ${dir}/Stats to ${TARFILE}"
        tar uvf ${TARFILE} ${dir}/Stats
    fi
done

gzip ${TARFILE}

REMOTEHOST=fireball.cs.umd.edu
REMOTEPATH=tmp
REMOTEUSER=tzannes

scp ${GZIPFILE} ${REMOTEUSER}@${REMOTEHOST}:${REMOTEPATH}
