#!/bin/sh
set -e

echo "--> Cloning and building SmartPianoEngine..."
rm -rf /tmp/engine /tmp/engine-install
git clone https://github.com/ProjetISIE/SmartPianoEngine.git /tmp/engine
cd /tmp/engine

cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF \
  -DCOVERAGE=OFF \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=clang-18 \
  -DCMAKE_CXX_COMPILER=clang++-18 \
  -DCMAKE_C_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
  -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -static-libstdc++ -fuse-ld=lld-18 -lasound -lpthread"
cmake --build build -j"$(nproc)"

# Engine's CMake install target is missing headers and libraries!
mkdir -p /tmp/engine-install/bin
mkdir -p /tmp/engine-install/include
mkdir -p /tmp/engine-install/lib
cp -r include/* /tmp/engine-install/include/
find build -name "*.a" -exec cp {} /tmp/engine-install/lib/ \;
cp build/src/main /tmp/engine-install/bin/engine

echo "--> Building SmartPianoLightUI..."
cd /workspace

cmake -B build-cross -DCMAKE_BUILD_TYPE=Release \
  -DENGINE_PATH=/tmp/engine-install \
  -DENGINE_INCLUDE_DIR=/tmp/engine-install/include \
  -DENGINE_LIBRARY=/tmp/engine-install/lib/libenginecomm.a \
  -DCMAKE_PREFIX_PATH=/usr/aarch64-linux-gnu \
  -DBUILD_TESTING=OFF \
  -DCOVERAGE=OFF \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=clang-18 \
  -DCMAKE_CXX_COMPILER=clang++-18 \
  -DCMAKE_C_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_COMPILER_TARGET=aarch64-linux-gnu \
  -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
  -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -static-libstdc++ -fuse-ld=lld-18 -lasound -lpthread"
cmake --build build-cross -j"$(nproc)"

echo "--> Packaging binaries..."
mkdir -p deploy
cp build-cross/src/main deploy/smart-piano-ui
cp /tmp/engine-install/bin/engine deploy/engine
chmod 777 deploy/smart-piano-ui deploy/engine
