@echo off

echo Remove prebuilt folder...
rd /S /Q prebuilt
echo Create prebuild folder...
md prebuilt

goto comment
echo Building openssl...
cd openssl-cmake
rd /S /Q cmake_build
md cmake_build
cd cmake_build
cmake -G Ninja -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../prebuilt ../
ninja
ninja install
cd ..
rd /S /Q cmake_build
cd ..
echo openssl built!
:comment

echo Building curl...
cd curl
rd /S /Q cmake_build
md cmake_build
cd cmake_build
cmake -G Ninja -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=Release -DCMAKE_USE_SCHANNEL=YES ^
  -DCMAKE_INSTALL_PREFIX:PATH=../../prebuilt ^
  ../
ninja
ninja install
cd ..
rd /S /Q cmake_build
cd ..
echo curl built!

echo Building glog...
cd glog
rd /S /Q cmake_build
md cmake_build
cd cmake_build
cmake -G Ninja -DWITH_GFLAGS=NO -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX:PATH=../../prebuilt ^
  ../
ninja
ninja install
cd ..
rd /S /Q cmake_build
cd ..
echo glog built!

echo Building yaml-cpp...
cd yaml-cpp
rd /S /Q cmake_build
md cmake_build
cd cmake_build
cmake -G Ninja -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX:PATH=../../prebuilt ^
  ../
ninja
ninja install
cd ..
rd /S /Q cmake_build
cd ..
echo yaml-cpp built!

echo All 3rdparty libs built!
