for NUM_THREAD in 1 2 4 8 16 32 64 128
do
    NUM_THREAD=${NUM_THREAD} make omp
    wait
    ./bin/host_code > profile/gaussian_OMP_nt${NUM_THREAD}.txt
    wait
    make clean
done