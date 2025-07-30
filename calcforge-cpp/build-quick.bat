@echo off
setlocal enabledelayedexpansion

echo Building CalcForge C++ with MSVC...
echo.

REM Direct path to vcvarsall.bat
echo Setting up MSVC environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to set up MSVC environment!
    pause
    exit /b 1
)

echo ✅ MSVC environment set up successfully!

REM Add Visual Studio CMake to PATH
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"

REM Add Qt6 to PATH  
set "PATH=C:\Qt\6.9.1\msvc2022_64\bin;%PATH%"

echo ✅ Tools added to PATH

REM Verify tools
echo.
echo Verifying tools...
cmake --version | findstr "cmake version"
qmake --version | findstr "Qt version"
cl 2>&1 | findstr "Microsoft"

echo.
echo Creating build directory...
if not exist build mkdir build
cd build

echo.
echo Running CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ✅ Build successful!
echo Executable: build\Release\CalcForge.exe
echo.
