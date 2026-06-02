Compile:
g++ -g -Wall src/*.cpp -lsndfile -lm -lfftw3f -o exe/main

Execute:
./exe/main /home/jon/Desktop/miralo.wav /home/jon/Desktop/miralo_mono_filtered.wav 8000
