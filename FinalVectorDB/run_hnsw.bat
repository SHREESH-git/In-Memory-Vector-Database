@echo off
rem Add MSYS64 UCRT64 compiler directory to the PATH
set PATH=C:\msys64\ucrt64\bin;%PATH%

echo Compiling HNSW.cpp...
g++ -std=c++17 -O3 HNSW.cpp -o HNSW.exe
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful. Running HNSW.exe...
    HNSW.exe
) else (
    echo Compilation failed.
    exit /b 1
)
