@echo off
setlocal enabledelayedexpansion

REM Create build log file with timestamp
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YY=%dt:~2,2%" & set "YYYY=%dt:~0,4%" & set "MM=%dt:~4,2%" & set "DD=%dt:~6,2%" & set "HH=%dt:~8,2%" & set "Min=%dt:~10,2%" & set "Sec=%dt:~12,2%"
set "timestamp=%YYYY%-%MM%-%DD%_%HH%-%Min%-%Sec%"
set "LOGFILE=build_log_%timestamp%.txt"

REM Function to echo and log
set "ECHOLOG=call :EchoLog"

echo Building CalcForge C++ with MSVC...
echo Building CalcForge C++ with MSVC... > "%LOGFILE%"
echo.
echo. >> "%LOGFILE%"
echo Build log will be saved to: %LOGFILE%
echo Build log will be saved to: %LOGFILE% >> "%LOGFILE%"
echo.
echo. >> "%LOGFILE%"

REM Direct path to vcvarsall.bat
echo Setting up MSVC environment...
echo Setting up MSVC environment... >> "%LOGFILE%"

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
