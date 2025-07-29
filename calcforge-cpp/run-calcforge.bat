@echo on
echo Running CalcForge with Qt environment...
echo Current directory: %CD%

REM Check if Qt directory exists
echo Checking Qt directory...
if exist "C:\Qt\6.9.1\msvc2022_64\bin" (
    echo Qt directory found: C:\Qt\6.9.1\msvc2022_64\bin
) else (
    echo ERROR: Qt directory not found: C:\Qt\6.9.1\msvc2022_64\bin
)

REM Check if CalcForge.exe exists
echo Checking CalcForge.exe...
if exist ".\build\Release\CalcForge.exe" (
    echo CalcForge.exe found: .\build\Release\CalcForge.exe
    dir ".\build\Release\CalcForge.exe"
) else (
    echo ERROR: CalcForge.exe not found: .\build\Release\CalcForge.exe
    echo Checking build directory contents:
    if exist ".\build\Release" (
        dir ".\build\Release\*.exe"
    ) else (
        echo Build\Release directory does not exist
    )
)

REM Set PATH and show it
echo Setting PATH...
set "PATH=C:\Qt\6.9.1\msvc2022_64\bin;C:\Windows\System32;C:\Windows"
echo New PATH: %PATH%

REM Check for required Qt DLLs
echo Checking for Qt DLLs...
if exist "C:\Qt\6.9.1\msvc2022_64\bin\Qt6Core.dll" (
    echo Qt6Core.dll found
) else (
    echo ERROR: Qt6Core.dll not found
)
if exist "C:\Qt\6.9.1\msvc2022_64\bin\Qt6Gui.dll" (
    echo Qt6Gui.dll found
) else (
    echo ERROR: Qt6Gui.dll not found
)
if exist "C:\Qt\6.9.1\msvc2022_64\bin\Qt6Widgets.dll" (
    echo Qt6Widgets.dll found
) else (
    echo ERROR: Qt6Widgets.dll not found
)

REM Try to run the application with timeout and error capture
echo Starting CalcForge...
echo Command: .\build\Release\CalcForge.exe
echo Waiting for application to start (timeout in 10 seconds)...

REM Use start command to run in background and capture output
start /wait "CalcForge" .\build\Release\CalcForge.exe
set EXITCODE=%ERRORLEVEL%

echo.
echo CalcForge process completed
echo CalcForge exit code: %EXITCODE%
if %EXITCODE% NEQ 0 (
    echo ERROR: CalcForge exited with error code %EXITCODE%
    echo This could indicate:
    echo - Missing DLL dependencies
    echo - Application crash during startup
    echo - Qt initialization failure
) else (
    echo CalcForge exited normally
)

echo.
echo Press any key to continue...
pause > nul
