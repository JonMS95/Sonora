Compile:
g++ -g -Wall src/*.cpp -lsndfile -lm -lfftw3f -lsqlite3 -o exe/main

Execute:
./exe/main /home/jon/Desktop/miralo.wav /home/jon/Desktop/miralo_mono_filtered.wav 8000 /home/jon/Desktop/scripts/Sonora/dat/fingerprints.db

Debug:
gdb --args ./exe/main /home/jon/Desktop/miralo.wav /home/jon/Desktop/miralo_mono_filtered.wav 8000 /home/jon/Desktop/scripts/Sonora/dat/fingerprints.db
