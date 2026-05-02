@echo off
chcp 65001 >nul
title Bhop External Build (LTO Optimized)
color 0A

echo ========================================
echo BHOP EXTERNAL - LTO BUILD
echo ========================================
echo.

REM --- Configuration ---
set PROJECT_DIR=%CD%

REM --- Clean previous build ---
if exist "bhop.exe" del "bhop.exe"
del *.obj *.pdb *.ilk *.exp 2>nul

echo [1/4] Setting up compiler environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %ERRORLEVEL% neq 0 (
    echo ERROR: Visual Studio vcvarsall.bat not found.
    pause
    exit /b 1
)

echo [2/4] Compiling with Whole Program Optimization (/GL)...
cl /std:c++20 /EHsc /O2 /MT /GL /Gw /DNDEBUG /c "%PROJECT_DIR%\bhop.cpp"

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo [3/4] Linking with LTCG and ICF...
:: Links the required libraries that were in your original g++ script: user32.lib and winmm.lib
link /OUT:bhop.exe bhop.obj user32.lib winmm.lib /SUBSYSTEM:CONSOLE /OPT:REF /OPT:ICF /LTCG

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Final linking failed!
    pause
    exit /b 1
)

echo [4/4] Cleaning up temporary files...
del *.obj *.ilk *.exp 2>nul

echo.
echo ========================================
echo LTO BUILD SUCCESSFUL!
echo ========================================
echo.
pause