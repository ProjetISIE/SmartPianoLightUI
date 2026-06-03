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

# =====================================================================
# The Engine's CMake install target is missing headers and libraries!
# We manually construct the exact folder structure the UI expects:
mkdir -p /tmp/engine-install/bin
mkdir -p /tmp/engine-install/include
mkdir -p /tmp/engine-install/lib

# 1. Copy the headers
cp -r include/* /tmp/engine-install/include/
# 2. Copy the static libraries
find build -name "*.a" -exec cp {} /tmp/engine-install/lib/ \;
# 3. Rename and copy the engine executable (which compiled as 'main')
cp build/src/main /tmp/engine-install/bin/engine
# =====================================================================

echo "--> Building SmartPianoLightUI..."
cd /workspace

# By explicitly passing ENGINE_INCLUDE_DIR and ENGINE_LIBRARY, we completely bypass
# CMake's cross-compilation path restrictions and force it to link the engine.
cmake -B build-cross -DCMAKE_BUILD_TYPE=Release \
  -DENGINE_PATH=/tmp/engine-install \
  -DENGINE_INCLUDE_DIR=/tmp/engine-install/include \
  -DENGINE_LIBRARY=/tmp/engine-install/lib/libenginecomm.a \
  -DCMAKE_PREFIX_PATH=/usr/aarch64-linux-gnu \
  -DBUILD_TESTING=OFF \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=clang-18 \
  -DCMAKE_CXX_COMPILER=clang++-18 \
  -DCMAKE_C_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
  -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -static-libstdc++ -lc++abi -fuse-ld=lld-18 -lasound -lpthread"
cmake --build build-cross -j"$(nproc)"

echo "--> Packaging binaries..."
mkdir -p deploy
cp build-cross/src/main deploy/smart-piano-ui
cp /tmp/engine-install/bin/engine deploy/engine
chmod +x deploy/smart-piano-ui deploy/engine
