@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
set PATH=C:\Qt\Tools\QtCreator\bin\jom;C:\Qt\6.11.1\msvc2022_64\bin;%PATH%
cmake -S . -B build/release -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/msvc2022_64" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DOPENSSL_NO_ASM=ON
cmake --build build/release --config Release
