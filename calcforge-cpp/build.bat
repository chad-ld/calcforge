@echo off
echo Building CalcForge C++ Hello World...
echo.

REM Create build directory
if not exist build mkdir build
cd build

REM Run CMake to generate build files
echo Running CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Make sure you have:
    echo - Visual Studio 2022 with C++ development tools
    echo - Qt6 installed and in PATH
    echo - CMake installed and in PATH
    pause
    exit /b 1
)

echo.
echo CMake configuration successful!
echo.

REM Build the project
echo Building project...
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
