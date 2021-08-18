#!/bin/bash
script_path="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "Removing prebuilt dir..."
rm -rf ${script_path}/prebuilt
echo "Creating prebuilt dir..."
mkdir -p ${script_path}/prebuilt

echo "Building openssl..."
cd ${script_path}/openssl
./config no-shared --prefix=${script_path}/prebuilt --openssldir=${script_path}/prebuilt
make -j4
make install
make clean
echo "openssl built!"

echo "Building curl..."
cd ${script_path}/curl
rm -rf cmake_build
mkdir cmake_build
cd cmake_build
cmake -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${script_path}/prebuilt ../
make -j4
make install
make clean
echo "curl built!"

echo "Building glog..."
cd ${script_path}/glog
rm -rf cmake_build
mkdir cmake_build
cd cmake_build
cmake -DWITH_GFLAGS=NO -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${script_path}/prebuilt ../
make -j4
make install
make clean
echo "glog built!"

echo "Building yaml-cpp..."
cd ${script_path}/yaml-cpp
rm -rf cmake_build
mkdir cmake_build
cd cmake_build
cmake -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${script_path}/prebuilt ../
make -j4
make install
make clean
echo "yaml-cpp built!"