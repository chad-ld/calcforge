@echo off
echo Starting CalcForge build...

REM Clean up log files
if exist cmake_configure.log del /f /q cmake_configure.log >nul 2>&1
if exist cmake_build.log del /f /q cmake_build.log >nul 2>&1

REM Check for MSVC environment
where cl >nul 2>&1
if %ERRORLEVEL% equ 0 goto build_start

REM Set up MSVC environment
echo Setting up MSVC environment...
set VCVARS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat
if not exist "%VCVARS_PATH%" (
    echo ERROR: vcvarsall.bat not found
    exit /b 1
)
call "%VCVARS_PATH%" x64 >nul 2>&1

:build_start
echo Creating build directory...
if not exist build mkdir build
cd build

echo Running CMake configure...
cmake .. -G "Visual Studio 17 2022" -A x64 >cmake_configure.log 2>&1

REM Check for CMake success
findstr /C:"Generating done" cmake_configure.log >nul
if %ERRORLEVEL% neq 0 (
    echo CMake configuration FAILED
    type cmake_configure.log
    exit /b 1
)
echo CMake configuration successful

echo Building project...
cmake --build . --config Release >cmake_build.log 2>&1

REM Check for build success
if %ERRORLEVEL% neq 0 (
    echo Build FAILED
    type cmake_build.log
    exit /b 1
)

echo Build completed successfully!
if exist "Release\CalcForge.exe" (
    echo Executable created: build\Release\CalcForge.exe
    dir "Release\CalcForge.exe"
) else (
    echo Warning: Executable not found
)
echo Done!
