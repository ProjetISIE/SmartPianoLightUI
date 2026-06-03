#!/bin/sh
set -e

echo "--> Cloning and building SmartPianoEngine..."
rm -rf /tmp/engine /tmp/engine-install
git clone https://github.com/ProjetISIE/SmartPianoEngine.git /tmp/engine
cd /tmp/engine

cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=clang-18 \
  -DCMAKE_CXX_COMPILER=clang++-18 \
  -DCMAKE_C_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
  -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -static-libstdc++ -lc++abi -fuse-ld=lld-18 -lasound -lpthread"
cmake --build build -j"$(nproc)"
cmake --install build --prefix /tmp/engine-install

echo "--> Building SmartPianoLightUI..."
cd /workspace

cmake -B build-cross -DCMAKE_BUILD_TYPE=Release \
  -DENGINE_PATH=/tmp/engine-install \
  -DCMAKE_PREFIX_PATH=/usr/aarch64-linux-gnu \
  -DBUILD_TESTING=OFF \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=clang-18 \
  -DCMAKE_CXX_COMPILER=clang++-18 \
  -DCMAKE_C_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
  -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -static-libstdc++ -lc++abi -fuse-ld=lld-18 -lasound -lpthread" \
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH \
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH
cmake --build build-cross -j"$(nproc)"
