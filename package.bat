@echo off
echo ==========================================
echo       Helix Automated Packaging Pipeline
echo ==========================================
echo.

echo [1/5] Compiling Helix (Release mode)...
call build_app.bat
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation failed!
    pause
    exit /b %ERRORLEVEL%
)
echo [SUCCESS] Compilation completed.
echo.

echo [2/5] Preparing dist directory...
if not exist dist mkdir dist
echo.

echo [3/5] Copying Helix.exe and PostgreSQL DLLs to dist...
copy /Y build\Desktop_Qt_6_11_1_MSVC2022_64bit-Release\Helix.exe dist\Helix.exe
if exist "C:\Program Files\PostgreSQL\17\bin" (
    echo Copying PostgreSQL 17 DLLs...
    copy /Y "C:\Program Files\PostgreSQL\17\bin\libpq.dll" dist\
    copy /Y "C:\Program Files\PostgreSQL\17\bin\libcrypto-3-x64.dll" dist\
    copy /Y "C:\Program Files\PostgreSQL\17\bin\libssl-3-x64.dll" dist\
    copy /Y "C:\Program Files\PostgreSQL\17\bin\libintl-9.dll" dist\
    copy /Y "C:\Program Files\PostgreSQL\17\bin\libiconv-2.dll" dist\
    copy /Y "C:\Program Files\PostgreSQL\17\bin\libwinpthread-1.dll" dist\
) else (
    echo [WARNING] PostgreSQL 17 bin directory not found. PostgreSQL DLLs were not copied.
)
echo.

echo [4/5] Running windeployqt to deploy dependencies...
"C:\Qt\6.11.1\msvc2022_64\bin\windeployqt.exe" --qmldir qml dist\Helix.exe
echo.

echo [5/5] Checking for Inno Setup compiler...
set ISCC="C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if exist %ISCC% (
    echo Compiling Helix_Setup.exe installer...
    %ISCC% setup.iss
    echo.
    echo ==========================================
    echo [SUCCESS] Helix_Setup.exe created successfully!
    echo ==========================================
) else (
    echo [INFO] Inno Setup compiler ISCC.exe not found at standard path.
    echo [INFO] Please open setup.iss in Inno Setup GUI to build your installer.
)
echo.
pause
