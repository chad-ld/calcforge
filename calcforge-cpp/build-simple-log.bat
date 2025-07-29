@echo off
echo Starting CalcForge build with detailed logging...

REM Check if VS BuildTools path exists
echo Checking Visual Studio BuildTools path...
set "VCVARS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VCVARS_PATH%" goto found_vcvars
echo ERROR: vcvarsall.bat not found at expected location
echo Expected: "%VCVARS_PATH%"
pause
exit /b 1

:found_vcvars
echo Found: vcvarsall.bat

REM Try to set up environment - skip if already in Developer Command Prompt
echo Setting up MSVC environment...
echo Checking if MSVC environment is already available...
where cl >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo MSVC environment already available - skipping setup
    echo === Environment Already Available === > build_setup.log
    goto env_ready
)

echo Setting up MSVC environment from vcvarsall.bat...
echo === Environment Setup Start === > build_setup.log
call "%VCVARS_PATH%" x64 >> build_setup.log 2>&1
echo === Environment Setup Complete === >> build_setup.log

if %ERRORLEVEL% neq 0 (
    echo FAILED: Could not set up MSVC environment (Error level: %ERRORLEVEL%)
    echo === Environment Setup Log ===
    type build_setup.log
    echo.
    echo TIP: Try running this script from a "Developer Command Prompt for VS 2022" instead
    pause
    exit /b 1
)

:env_ready

echo Environment setup successful, starting build...

REM Create build directory and navigate
echo Creating build directory...
if not exist build mkdir build
cd build
echo Changed to build directory: %CD%

REM Run CMake configure with logging
echo Running CMake configure...
echo === CMake Configure Start === > cmake_configure.log
cmake .. -G "Visual Studio 17 2022" -A x64 >> cmake_configure.log 2>&1
echo === CMake Configure Complete === >> cmake_configure.log

REM Check if CMake actually succeeded by looking for success indicators
findstr /C:"Generating done" cmake_configure.log >nul
if %ERRORLEVEL% equ 0 (
    echo CMake configuration successful!
) else (
    echo FAILED: CMake configuration failed
    echo === CMake Configuration Errors ===
    type cmake_configure.log
    pause
    exit /b 1
)

echo CMake configure successful, starting build...

REM Run the actual build with logging
echo Building project...
echo === Build Start === > cmake_build.log
cmake --build . --config Release >> cmake_build.log 2>&1
echo === Build Complete === >> cmake_build.log

if %ERRORLEVEL% neq 0 (
    echo FAILED: Build failed with errors (Error level: %ERRORLEVEL%)
    echo === Build Errors ===
    type cmake_build.log
    pause
    exit /b 1
)

echo Build completed successfully!
echo Executable should be at: build\Release\CalcForge.exe

pause