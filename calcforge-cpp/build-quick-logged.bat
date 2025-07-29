@echo off
setlocal enabledelayedexpansion

REM Create build log file with timestamp
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YY=%dt:~2,2%" & set "YYYY=%dt:~0,4%" & set "MM=%dt:~4,2%" & set "DD=%dt:~6,2%" & set "HH=%dt:~8,2%" & set "Min=%dt:~10,2%" & set "Sec=%dt:~12,2%"
set "timestamp=%YYYY%-%MM%-%DD%_%HH%-%Min%-%Sec%"
set "LOGFILE=build_log_%timestamp%.txt"

echo ===============================================
echo CalcForge C++ Build Script with Logging
echo Build log will be saved to: %LOGFILE%
echo ===============================================
echo.

REM Redirect all output to both console and log file using PowerShell
powershell -Command "& {
    Write-Host 'Building CalcForge C++ with MSVC...' -ForegroundColor Green
    Write-Host ''
    
    Write-Host 'Setting up MSVC environment...' -ForegroundColor Yellow
    $env:PATH = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;' + $env:PATH
    $env:PATH = 'C:\Qt\6.9.1\msvc2022_64\bin;' + $env:PATH
    
    # Initialize MSVC environment
    & 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat' x64
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host 'ERROR: Failed to set up MSVC environment!' -ForegroundColor Red
        exit 1
    }
    
    Write-Host '✅ MSVC environment set up successfully!' -ForegroundColor Green
    Write-Host '✅ Tools added to PATH' -ForegroundColor Green
    Write-Host ''
    
    Write-Host 'Verifying tools...' -ForegroundColor Yellow
    cmake --version | Select-String 'cmake version'
    qmake --version | Select-String 'Qt version'  
    cl 2>&1 | Select-String 'Microsoft'
    Write-Host ''
    
    Write-Host 'Creating build directory...' -ForegroundColor Yellow
    if (-not (Test-Path 'build')) { New-Item -ItemType Directory -Name 'build' }
    Set-Location 'build'
    
    Write-Host ''
    Write-Host 'Running CMake...' -ForegroundColor Yellow
    cmake .. -G 'Visual Studio 17 2022' -A x64
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host 'ERROR: CMake configuration failed!' -ForegroundColor Red
        Read-Host 'Press Enter to continue...'
        exit 1
    }
    
    Write-Host ''
    Write-Host 'Building project...' -ForegroundColor Yellow
    cmake --build . --config Release
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host 'ERROR: Build failed!' -ForegroundColor Red
        Read-Host 'Press Enter to continue...'
        exit 1  
    }
    
    Write-Host ''
    Write-Host '✅ Build successful!' -ForegroundColor Green
    Write-Host 'Executable: build\Release\CalcForge.exe' -ForegroundColor Green
    Write-Host ''
    
}" 2>&1 | powershell -Command "Tee-Object -FilePath '%LOGFILE%'"

echo.
echo Build complete! Log saved to: %LOGFILE%
pause