@echo off
echo Testing Visual Studio detection...

echo Checking: C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    echo ✅ FOUND: Visual Studio Build Tools in x86 Program Files
    set "VCVARSALL=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
) else (
    echo ❌ NOT FOUND: Visual Studio Build Tools in x86 Program Files
)

if defined VCVARSALL (
    echo SUCCESS: VCVARSALL=%VCVARSALL%
) else (
    echo FAILED: VCVARSALL not set
)

pause
