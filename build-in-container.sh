#!/bin/sh
set -e

echo "--> Cloning and building SmartPianoEngine..."
# Clean up previous engine build if it exists in the container's temp space
rm -rf /tmp/engine /tmp/engine-install
git clone https://github.com/ProjetISIE/SmartPianoEngine.git /tmp/engine
cd /tmp/engine

cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
cmake --build build -j"$(nproc)"
cmake --install build --prefix /tmp/engine-install

echo "--> Building SmartPianoLightUI..."
cd /workspace
cmake -B build-cross -DCMAKE_BUILD_TYPE=Release \
  -DENGINE_PATH=/tmp/engine-install \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
cmake --build build-cross -j"$(nproc)"

echo "--> Packaging binaries..."
mkdir -p deploy
cp build-cross/main deploy/smart-piano-ui
cp /tmp/engine-install/bin/engine deploy/engine
chmod +x deploy/smart-piano-ui deploy/engine
