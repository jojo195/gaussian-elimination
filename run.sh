#!/bin/bash
for ALGORITHM in INTUITIVE INTERLEAVE PIPELINE
do
    for NUM_THREAD in 1 2 4 8 16 32 64 128
    do
        ALGORITHM=${ALGORITHM} NUM_THREAD=${NUM_THREAD} make all
        wait
        ./bin/host_code > profile/gaussian_${ALGORITHM}_nt${NUM_THREAD}.txt
        wait
        make clean
    done
done