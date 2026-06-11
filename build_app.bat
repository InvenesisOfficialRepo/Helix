@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
set PATH=C:\Qt\Tools\QtCreator\bin\jom;%PATH%
cmake --build build/Desktop_Qt_6_11_1_MSVC2022_64bit-Release --config Release
