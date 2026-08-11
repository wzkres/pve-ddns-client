@echo off

if "%~1" == "" (
    set "BUILD_CONFIG=Release"
    echo "Release build..."
) else (
    set "BUILD_CONFIG=%~1"
    echo "%~1 build..."
)

echo Remove prebuilt folder...
rd /S /Q prebuilt
echo Create prebuild folder...
md prebuilt

goto comment
echo Building openssl...
cd openssl-cmake
rd /S /Q cmake_build
cmake -B ./cmake_build -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=%BUILD_CONFIG% -DCMAKE_INSTALL_PREFIX=../../prebuilt
cmake --build ./cmake_build --config %BUILD_CONFIG%
cmake --install ./cmake_build --config %BUILD_CONFIG%
cd ..
rd /S /Q cmake_build
cd ..
echo openssl built!
:comment

echo Building curl...
cd curl
rd /S /Q cmake_build
cmake -B ./cmake_build -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=%BUILD_CONFIG% -DCMAKE_USE_SCHANNEL=YES ^
  -DCMAKE_INSTALL_PREFIX:PATH=../prebuilt
cmake --build ./cmake_build --config %BUILD_CONFIG%
cmake --install ./cmake_build --config %BUILD_CONFIG%
rd /S /Q cmake_build
cd ..
echo curl built!

echo Building fmt...
cd fmt
rd /S /Q cmake_build
cmake -B ./cmake_build -DBUILD_SHARED_LIBS=NO -DFMT_DOC=NO -DFMT_TEST=NO -DCMAKE_BUILD_TYPE=%BUILD_CONFIG% ^
  -DCMAKE_INSTALL_PREFIX:PATH=../prebuilt
cmake --build ./cmake_build --config %BUILD_CONFIG%
cmake --install ./cmake_build --config %BUILD_CONFIG%
rd /S /Q cmake_build
cd ..
echo fmt built!

echo Building spdlog...
cd spdlog
rd /S /Q cmake_build
cmake -B ./cmake_build -DBUILD_SHARED_LIBS=NO -DSPDLOG_FMT_EXTERNAL=YES -DCMAKE_BUILD_TYPE=%BUILD_CONFIG% ^
  -DCMAKE_INSTALL_PREFIX:PATH=../prebuilt
cmake --build ./cmake_build --config %BUILD_CONFIG%
cmake --install ./cmake_build --config %BUILD_CONFIG%
rd /S /Q cmake_build
cd ..
echo spdlog built!

echo Building yaml-cpp...
cd yaml-cpp
rd /S /Q cmake_build
cmake -B ./cmake_build -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=%BUILD_CONFIG% ^
  -DCMAKE_INSTALL_PREFIX:PATH=../prebuilt
cmake --build ./cmake_build --config %BUILD_CONFIG%
cmake --install ./cmake_build --config %BUILD_CONFIG%
rd /S /Q cmake_build
cd ..
echo yaml-cpp built!

echo All 3rdparty libs built!
