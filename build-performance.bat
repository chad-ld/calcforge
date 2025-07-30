@echo off
setlocal enabledelayedexpansion

echo Building CalcForge C++ for MAXIMUM PERFORMANCE...
echo.

REM Setup MSVC environment
echo Setting up MSVC environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to set up MSVC environment!
    pause
    exit /b 1
)

REM Add tools to PATH
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"
set "PATH=C:\Qt\6.9.1\msvc2022_64\bin;%PATH%"

echo ✅ Performance build environment ready

REM Clean previous build
if exist build rmdir /s /q build
mkdir build
cd build

echo.
echo Configuring for MAXIMUM PERFORMANCE...

REM Configure with performance optimizations
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CXX_FLAGS_RELEASE="/O2 /Ob2 /DNDEBUG /GL /arch:AVX2" ^
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="/LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS"

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Building with MAXIMUM OPTIMIZATIONS...

REM Build with maximum optimization
cmake --build . --config Release --parallel

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ✅ PERFORMANCE BUILD SUCCESSFUL!
echo.

REM Show file size
for %%F in (Release\CalcForge.exe) do (
    echo Executable size: %%~zF bytes
)

echo.
echo Performance optimizations applied:
echo ✅ Whole Program Optimization (/GL)
echo ✅ Link Time Code Generation (/LTCG)
echo ✅ Maximum Speed Optimization (/O2)
echo ✅ Inline Function Expansion (/Ob2)
echo ✅ Dead Code Elimination (/OPT:REF /OPT:ICF)
echo ✅ AVX2 Instructions (/arch:AVX2)
echo ✅ Qt Debug Output Disabled
echo.

set /p run="Test performance build? (y/n): "
if /i "%run%"=="y" (
    echo.
    echo Launching performance-optimized CalcForge...
    echo Measuring startup time...
    
    REM Add Qt to PATH and run
    set "PATH=C:\Qt\6.9.1\msvc2022_64\bin;%PATH%"
    
    echo Start time: %time%
    Release\CalcForge.exe
    echo End time: %time%
)

pause
