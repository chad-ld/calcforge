@echo off
echo Building CalcForge C++ with MSVC...
echo.

REM Check if MSVC is available by looking for cl.exe
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Setting up MSVC environment...
    
    REM Try to find and run vcvarsall.bat
    set "VCVARSALL="
    
    REM Check common Visual Studio 2022 locations
    set "VCVARSALL="

    REM Check x86 Program Files first (where Build Tools usually installs)
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"

    REM Check regular Program Files locations
    if "%VCVARSALL%"=="" if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    if "%VCVARSALL%"=="" if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    if "%VCVARSALL%"=="" if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    if "%VCVARSALL%"=="" if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
    
    if "%VCVARSALL%"=="" (
        echo ERROR: Visual Studio 2022 not found!
        echo.
        echo Please install one of:
        echo 1. Visual Studio 2022 Community (free)
        echo 2. Visual Studio 2022 Build Tools (minimal)
        echo.
        echo Download from: https://visualstudio.microsoft.com/downloads/
        echo.
        pause
        exit /b 1
    )
    
    echo Found Visual Studio at: %VCVARSALL%
    call "%VCVARSALL%" x64
    
    if %ERRORLEVEL% neq 0 (
        echo ERROR: Failed to set up MSVC environment!
        pause
        exit /b 1
    )
)

REM Check if CMake is available
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo CMake not in PATH, checking Visual Studio CMake...

    REM Check for Visual Studio's CMake
    set "VS_CMAKE="
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "VS_CMAKE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        echo ✅ Found Visual Studio CMake
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        echo ✅ Found Visual Studio CMake
    )

    if "%VS_CMAKE%"=="" (
        echo ERROR: CMake not found!
        echo Please install CMake from https://cmake.org/download/
        pause
        exit /b 1
    )

    REM Add Visual Studio CMake to PATH for this session
    set "PATH=%VS_CMAKE%;%PATH%"
) else (
    echo ✅ CMake found in PATH
)

REM Check if Qt6 is available
where qmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo WARNING: Qt6 qmake not found in PATH!
    echo Make sure Qt6 MSVC version is installed and added to PATH.
    echo Example: C:\Qt\6.5.0\msvc2022_64\bin
    echo.
)

echo Tools check passed!
echo.

REM Create build directory
if not exist build mkdir build
cd build

REM Run CMake to generate build files
echo Running CMake with Visual Studio generator...
cmake .. -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Make sure Qt6 MSVC version is properly installed and in PATH.
    pause
    exit /b 1
)

echo.
echo CMake configuration successful!
echo.

REM Build the project
echo Building project with MSVC...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo.
echo Executable location: build\Release\CalcForge.exe
echo.

REM Ask if user wants to run the application
set /p run="Run the application now? (y/n): "
if /i "%run%"=="y" (
    echo.
    echo Running CalcForge Hello World...
    Release\CalcForge.exe
)

pause
