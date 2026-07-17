#!/usr/bin/env bash
set -e

echo "Installing Ubuntu packages..."

sudo apt update
sudo apt install -y \
    build-essential \
    g++ \
    make \
    cmake \
    git \
    libsamplerate0-dev \
    libsndfile1-dev \
    libfftw3-dev \
    libsqlite3-dev

echo "Installing Catch2 v2.13.10..."

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

git clone https://github.com/catchorg/Catch2.git "$tmpdir/Catch2"
cd "$tmpdir/Catch2"

git checkout v2.13.10

cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF

cmake --build build -j"$(nproc)"
sudo cmake --install build

echo "Dependencies installed successfully."
