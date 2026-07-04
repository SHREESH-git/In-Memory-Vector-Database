@echo off
rem Add MSYS64 UCRT64 compiler directory to the PATH
set PATH=C:\msys64\ucrt64\bin;%PATH%

echo Compiling Generate_Dataset.cpp...
g++ -std=c++17 -O3 Generate_Dataset.cpp -o Generate_Dataset.exe
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful. Running Generate_Dataset.exe...
    Generate_Dataset.exe
) else (
    echo Compilation failed.
    exit /b 1
)
