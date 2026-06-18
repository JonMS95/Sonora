Install dependencies (if needed):
sudo apt install libsamplerate0-dev libsndfile1-dev libfftw3-dev libsqlite3-dev

Create directories:
mkdir dat exe

Compile:
g++ -g -Wall src/*.cpp -lsamplerate -lsndfile -lm -lfftw3f -lsqlite3 -o exe/main

Execute (index):
./exe/main i /home/jon/Desktop/test_sonora/in/file.wav /home/jon/Desktop/scripts/Sonora/dat/fingerprints.db 1000 21 0.1 4 3 3

Execute (match):
./exe/main m /home/jon/Desktop/test_sonora/in/file.wav /home/jon/Desktop/scripts/Sonora/dat/fingerprints.db 1000 21 0.1 4 3 3

Execute (many index ops):
for file in $(ls /home/jon/Desktop/test_sonora/in/); do echo -e "----------\r\n${file}"; time ./exe/main i /home/jon/Desktop/test_sonora/in/${file} /home/jon/Desktop/scripts/Sonora/dat/fingerprints.db 1000 21 0.1 4 3 3; echo -e "----------\r\n"; done

Execute (many match ops):
for file in $(ls /home/jon/Desktop/test_sonora/in/); do echo -e "----------\r\n${file}\r\n$(basename $(./exe/main m /home/jon/Desktop/test_sonora/in/${file} /home/jon/Desktop/scripts/Sonora/dat/fingerprints.db 1000 21 0.1 4 3 3))\r\n----------\r\n"; done

Debug:
gdb --args ./exe/main /home/jon/Desktop/test_sonora/in/miralo.wav /home/jon/Desktop/test_sonora/out/miralo.wav 8000 /home/jon/Desktop/scripts/Sonora/dat/fingerprints.db

Check database content:
sqlite3 dat/fingerprints.db -table "SELECT * FROM fingerprints WHERE frame_idx = 100 LIMIT 10;"

Check output file sample rate (if any):
ffprobe -v error -select_streams a:0 -show_entries stream=sample_rate -of default=noprint_wrappers=1 file_name.wav

Get some free samples:
https://www.freesound.org/search/?s=Duration+%28shortest+first%29&f=category%3A%22Music%22&page=500#sound
