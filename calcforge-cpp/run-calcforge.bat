@echo off
echo Running CalcForge with Qt environment...

REM Add Qt6 to PATH for this session
set "PATH=C:\Qt\6.9.1\msvc2022_64\bin;%PATH%"

REM Run the application
echo Starting CalcForge...
.\build\Release\CalcForge.exe

echo.
echo CalcForge closed.
