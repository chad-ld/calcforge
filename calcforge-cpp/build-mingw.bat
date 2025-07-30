@echo off
echo Building CalcForge C++ with MinGW...
echo.

REM Check if MinGW is available
where g++ >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: MinGW g++ compiler not found!
    echo.
    echo Please install MinGW-w64 via MSYS2:
    echo 1. Download MSYS2 from https://www.msys2.org/
    echo 2. Install and run: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
    echo 3. Add C:\msys64\mingw64\bin to your PATH
    echo.
    pause
    exit /b 1
)

REM Check if CMake is available
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake not found!
    echo.
    echo Please install CMake from https://cmake.org/download/
    echo Make sure to add it to your PATH during installation.
    echo.
    pause
    exit /b 1
)

REM Check if Qt6 is available
where qmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo WARNING: Qt6 qmake not found in PATH!
    echo Make sure Qt6 is installed and added to PATH.
    echo Example: C:\Qt\6.5.0\mingw_64\bin
    echo.
)

echo Tools check passed!
echo.

REM Create build directory
if not exist build mkdir build
cd build

REM Run CMake to generate build files
echo Running CMake with MinGW Makefiles...
cmake .. -G "MinGW Makefiles"

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Make sure Qt6 is properly installed and in PATH.
    pause
    exit /b 1
)

echo.
echo CMake configuration successful!
echo.

REM Build the project
echo Building project with MinGW...
cmake --build .

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo.
echo Executable location: build\CalcForge.exe
echo.

REM Ask if user wants to run the application
set /p run="Run the application now? (y/n): "
if /i "%run%"=="y" (
    echo.
    echo Running CalcForge Hello World...
    CalcForge.exe
)

pause
