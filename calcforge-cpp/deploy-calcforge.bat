@echo off
echo Deploying CalcForge with Qt libraries...

REM Add Qt6 to PATH for this session
set "PATH=C:\Qt\6.9.1\msvc2022_64\bin;%PATH%"

REM Create deployment directory
if not exist deploy mkdir deploy
if not exist deploy\CalcForge mkdir deploy\CalcForge

REM Copy the executable
copy "build\Release\CalcForge.exe" "deploy\CalcForge\"

REM Copy data files for portable operation
echo Copying data files...
if exist "worksheets.json" copy "worksheets.json" "deploy\CalcForge\"
if exist "example_worksheets.json" copy "example_worksheets.json" "deploy\CalcForge\"
if exist "combined_worksheets.json" copy "combined_worksheets.json" "deploy\CalcForge\"
if exist "comprehensive_worksheets.json" copy "comprehensive_worksheets.json" "deploy\CalcForge\"
if exist "exchange_rates.json" copy "exchange_rates.json" "deploy\CalcForge\"
if exist "api_key_example.txt" copy "api_key_example.txt" "deploy\CalcForge\"

REM Use Qt's deployment tool to copy required DLLs
echo Running windeployqt to copy Qt libraries...
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw "deploy\CalcForge\CalcForge.exe"

if %ERRORLEVEL% neq 0 (
    echo ERROR: windeployqt failed!
    echo Make sure Qt6 is in your PATH.
    exit /b 1
)

REM Create a portable ZIP package
echo Creating portable ZIP package...
if exist "deploy\CalcForge-Portable.zip" del "deploy\CalcForge-Portable.zip"
powershell -command "Compress-Archive -Path 'deploy\CalcForge\*' -DestinationPath 'deploy\CalcForge-Portable.zip'"

echo.
echo ✅ Deployment successful!
echo.
echo Standalone CalcForge is ready in: deploy\CalcForge\
echo Portable ZIP package created: deploy\CalcForge-Portable.zip
echo You can now run CalcForge.exe from that folder without needing Qt in PATH.
echo.

echo.
echo To run the deployed version: deploy\CalcForge\CalcForge.exe
echo Deployment complete!
