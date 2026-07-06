Install dependencies (if needed):
sudo apt install libsamplerate0-dev libsndfile1-dev libfftw3-dev libsqlite3-dev catch2

Compile library and CLI executable:
make release

Execute (index):
./exe/release/sonora-cli i songs/song_0.wav data/fingerprints.db 16000 51 0.05 5 5 5

Execute (match):
./exe/release/sonora-cli m samples/sample_0.wav data/fingerprints.db 16000 51 0.05 5 5 5

Get some free samples:
https://www.freesound.org/search/?s=Duration+%28shortest+first%29&f=category%3A%22Music%22&page=500#sound
