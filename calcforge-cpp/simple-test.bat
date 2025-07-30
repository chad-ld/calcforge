@echo off
echo Simple VS Test...

set "TESTPATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
echo Testing path: %TESTPATH%

if exist "%TESTPATH%" (
    echo SUCCESS: File exists!
    set "VCVARSALL=%TESTPATH%"
    echo VCVARSALL is set to: %VCVARSALL%
) else (
    echo FAILED: File does not exist
)

pause
