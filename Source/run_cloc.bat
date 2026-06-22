@echo off
cd /d "%~dp0"
echo Counting C++ and Header lines of code...
echo ----------------------------------------
cloc --include-ext=cpp,h .
echo ----------------------------------------
pause
