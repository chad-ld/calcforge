@echo off
echo Starting CalcForge Development Environment...
echo.

REM Check if Node.js is installed
node --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Node.js is not installed or not in PATH
    echo Please install Node.js from https://nodejs.org/
    pause
    exit /b 1
)

REM Check if Python is installed
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python from https://python.org/
    pause
    exit /b 1
)

REM Check if npm dependencies are installed
if not exist "node_modules" (
    echo Installing npm dependencies...
    npm install
    if %errorlevel% neq 0 (
        echo ERROR: Failed to install npm dependencies
        pause
        exit /b 1
    )
)

REM Check if Python dependencies are installed
echo Checking Python dependencies...
cd backend
python -c "import fastapi, uvicorn, pint, requests" >nul 2>&1
if %errorlevel% neq 0 (
    echo Installing Python dependencies...
    pip install -r requirements.txt
    if %errorlevel% neq 0 (
        echo ERROR: Failed to install Python dependencies
        cd ..
        pause
        exit /b 1
    )
)
cd ..

echo.
echo Starting CalcForge...
echo Backend will start on http://localhost:8000
echo Electron app will launch automatically
echo.
echo Press Ctrl+C to stop both processes
echo.

REM Start development mode
npm run dev
