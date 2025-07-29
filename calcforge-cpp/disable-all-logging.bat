@echo off
echo Temporarily disabling ALL debug logging to test GUI...

REM Create a backup of Logger.h
copy "include\Logger.h" "include\Logger.h.bak"

REM Replace LOG_DEBUG with empty macro
powershell -Command "(Get-Content 'include\Logger.h') -replace '#define LOG_DEBUG\(message\) Logger::instance\(\).debug\(message\)', '#define LOG_DEBUG(message) // DISABLED' | Set-Content 'include\Logger.h'"

echo Debug logging disabled. Build and test the application.
echo To re-enable logging, run: copy "include\Logger.h.bak" "include\Logger.h"
pause