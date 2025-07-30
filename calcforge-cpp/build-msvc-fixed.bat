@echo off
echo Building CalcForge C++ with MSVC...
echo.

REM Check if MSVC is available by looking for cl.exe
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Setting up MSVC environment...
    
    REM Set the path to vcvarsall.bat (we know it exists here)
    set "VCVARSALL=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
    
    echo Found Visual Studio at: %VCVARSALL%
    call "%VCVARSALL%" x64
    
    if %ERRORLEVEL% neq 0 (
        echo ERROR: Failed to set up MSVC environment!
        pause
        exit /b 1
    )
    
    echo ✅ MSVC environment set up successfully!
)

REM Check if CMake is available
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo CMake not in PATH, using Visual Studio CMake...
    
    REM Add Visual Studio CMake to PATH for this session
    set "VS_CMAKE_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    set "PATH=%VS_CMAKE_PATH%;%PATH%"
    echo ✅ Added Visual Studio CMake to PATH
) else (
    echo ✅ CMake found in PATH
)

REM Check if Qt6 is available
where qmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Qt6 not in PATH, adding Qt6 MSVC to PATH...
    
    REM Add Qt6 MSVC to PATH for this session
    set "QT_PATH=C:\Qt\6.9.1\msvc2022_64\bin"
    set "PATH=%QT_PATH%;%PATH%"
    echo ✅ Added Qt6 MSVC to PATH
) else (
    echo ✅ Qt6 found in PATH
)

echo.
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
    echo Make sure Qt6 MSVC version is properly installed.
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
