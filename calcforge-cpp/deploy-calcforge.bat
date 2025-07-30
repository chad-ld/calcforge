@echo off
echo Deploying CalcForge with Qt libraries...

REM Add Qt6 to PATH for this session
set "PATH=C:\Qt\6.9.1\msvc2022_64\bin;%PATH%"

REM Create deployment directory
if not exist deploy mkdir deploy
if not exist deploy\CalcForge mkdir deploy\CalcForge

REM Copy the executable
copy "build\Release\CalcForge.exe" "deploy\CalcForge\"

REM Use Qt's deployment tool to copy required DLLs
echo Running windeployqt to copy Qt libraries...
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw "deploy\CalcForge\CalcForge.exe"

if %ERRORLEVEL% neq 0 (
    echo ERROR: windeployqt failed!
    echo Make sure Qt6 is in your PATH.
    pause
    exit /b 1
)

echo.
echo ✅ Deployment successful!
echo.
echo Standalone CalcForge is ready in: deploy\CalcForge\
echo You can now run CalcForge.exe from that folder without needing Qt in PATH.
echo.

set /p run="Run the deployed version? (y/n): "
if /i "%run%"=="y" (
    echo.
    echo Running deployed CalcForge...
    deploy\CalcForge\CalcForge.exe
)

pause
