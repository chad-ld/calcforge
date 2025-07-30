@echo off
setlocal enabledelayedexpansion

echo Building CalcForge C++ for MAXIMUM PERFORMANCE...
echo.

REM Clean build
if exist build rmdir /s /q build
mkdir build
cd build

REM Setup environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"
set "PATH=C:\Qt\6.9.1\msvc2022_64\bin;%PATH%"

echo ✅ Environment ready

echo.
echo Configuring with performance optimizations...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake failed!
    pause
    exit /b 1
)

echo.
echo Building with maximum optimizations...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ✅ PERFORMANCE BUILD COMPLETE!
echo.

for %%F in (Release\CalcForge.exe) do (
    echo Executable: %%~fF
    echo Size: %%~zF bytes
)

echo.
echo Performance build ready! Use run-performance.bat to test.
pause
