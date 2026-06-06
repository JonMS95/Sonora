Compile:
g++ -g -Wall src/*.cpp -lsamplerate -lsndfile -lm -lfftw3f -lsqlite3 -o exe/main

Execute:
./exe/main /home/jon/Desktop/test_sonora/in/miralo.wav /home/jon/Desktop/test_sonora/out/miralo.wav 8000 /home/jon/Desktop/scripts/Sonora/dat/fingerprints.db

Debug:
gdb --args ./exe/main /home/jon/Desktop/miralo.wav /home/jon/Desktop/miralo_mono_filtered.wav 8000 /home/jon/Desktop/scripts/Sonora/dat/fingerprints.db

Check database content:
sqlite3 dat/fingerprints.db -table "SELECT * FROM fingerprints WHERE frame_idx = 100 LIMIT 10;"

Check output file sample rate (if any):
ffprobe -v error -select_streams a:0 -show_entries stream=sample_rate -of default=noprint_wrappers=1 file_name.wav