@echo off
echo Checking CalcForge C++ Development Tools...
echo.

echo ========================================
echo Checking CMake...
echo ========================================
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ⚠️  CMake not in PATH, checking Visual Studio CMake...

    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        echo ✅ Found Visual Studio CMake (x86)
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --version | findstr "cmake version"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        echo ✅ Found Visual Studio CMake
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --version | findstr "cmake version"
    ) else (
        echo ❌ CMake not found anywhere
        echo Please install CMake from https://cmake.org/download/
    )
) else (
    echo ✅ CMake found in PATH:
    cmake --version
)
echo.

echo ========================================
echo Checking Qt6...
echo ========================================
where qmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ❌ Qt6 qmake not found in PATH
    echo Please install Qt6 MSVC version from https://www.qt.io/download
    echo Add C:\Qt\6.5.0\msvc2022_64\bin to your PATH
) else (
    echo ✅ Qt6 found:
    qmake --version
)
echo.

echo ========================================
echo Checking MSVC Compiler...
echo ========================================
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ⚠️  MSVC not in PATH (this is normal)
    echo Checking for Visual Studio installations...
    
    set "FOUND_VS=0"
    
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        echo ✅ Found Visual Studio 2022 Community
        set "FOUND_VS=1"
    )
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
        echo ✅ Found Visual Studio 2022 Professional  
        set "FOUND_VS=1"
    )
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        echo ✅ Found Visual Studio 2022 Enterprise
        set "FOUND_VS=1"
    )
    if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        echo ✅ Found Visual Studio 2022 Build Tools
        set "FOUND_VS=1"
    )
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        echo ✅ Found Visual Studio 2022 Build Tools (x86)
        set "FOUND_VS=1"
    )
    
    if "%FOUND_VS%"=="0" (
        echo ❌ No Visual Studio 2022 installation found
        echo Please install Build Tools for Visual Studio 2022
        echo Download from: https://visualstudio.microsoft.com/downloads/
    )
) else (
    echo ✅ MSVC compiler found:
    cl 2>&1 | findstr "Microsoft"
)
echo.

echo ========================================
echo Summary
echo ========================================
echo If all tools show ✅, you're ready to build!
echo If any show ❌, please install the missing tools.
echo.
echo Next steps:
echo 1. Run: build-msvc.bat
echo 2. Or open in VS Code: code .
echo.
pause
