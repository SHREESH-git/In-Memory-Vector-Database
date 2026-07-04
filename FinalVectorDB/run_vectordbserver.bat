@echo off
rem Add MSYS64 UCRT64 compiler directory to the PATH
set PATH=C:\msys64\ucrt64\bin;%PATH%

echo Compiling VectorDBServer.cpp...
g++ -std=c++17 -O3 -D_WIN32_WINNT=0x0A00 VectorDBServer.cpp -o VectorDBServer.exe -lws2_32
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful. Running VectorDBServer.exe...
    VectorDBServer.exe
) else (
    echo Compilation failed.
    exit /b 1
)
