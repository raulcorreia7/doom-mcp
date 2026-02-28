@echo off
REM download_wad.bat - Download Doom shareware WAD
REM
REM Usage: download_wad.bat [OUTPUT_DIR]
REM
REM Downloads doom1.wad (shareware) to OUTPUT_DIR (default: current dir).
REM Requires bitsadmin or certutil (built into Windows 7+).

setlocal enabledelayedexpansion

set "OUT_DIR=%~1"
if "%OUT_DIR%"=="" set "OUT_DIR=."
set "OUT_FILE=%OUT_DIR%\doom1.wad"

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM Check if already exists and is valid (>=4MB)
if exist "%OUT_FILE%" (
    for %%F in ("%OUT_FILE%") do set "SIZE=%%~zF"
    if !SIZE! GEQ 4000000 (
        echo doom1.wad already exists ^(!SIZE! bytes^)
        exit /b 0
    )
    del "%OUT_FILE%" 2>nul
)

REM URLs to try
set "URL1=https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad"
set "URL2=https://raw.githubusercontent.com/Doom-Utils/shareware-collection/master/Doom%%201.0/doom1.wad"
set "URL3=https://archive.org/download/DoomsharewareEpisode/doom1.wad"

REM Try bitsadmin first, then certutil
where bitsadmin >nul 2>&1 && goto :use_bitsadmin
where certutil >nul 2>&1 && goto :use_certutil
echo error: need bitsadmin or certutil
exit /b 1

:use_bitsadmin
echo Downloading doom1.wad...
for %%U in (%URL1% %URL2% %URL3%) do (
    bitsadmin /transfer wad /download /priority normal "%%~U" "%OUT_FILE%" >nul 2>&1
    if exist "%OUT_FILE%" for %%F in ("%OUT_FILE%") do if %%~zF GEQ 4000000 (
        echo Downloaded: %OUT_FILE%
        exit /b 0
    )
    del "%OUT_FILE%" 2>nul
)
goto :failed

:use_certutil
echo Downloading doom1.wad...
for %%U in (%URL1% %URL2% %URL3%) do (
    certutil -urlcache -split -f "%%~U" "%OUT_FILE%" >nul 2>&1
    if exist "%OUT_FILE%" for %%F in ("%OUT_FILE%") do if %%~zF GEQ 4000000 (
        echo Downloaded: %OUT_FILE%
        exit /b 0
    )
    del "%OUT_FILE%" 2>nul
)
goto :failed

:failed
echo Failed to download. Get doom1.wad from:
echo   https://archive.org/details/DoomsharewareEpisode
exit /b 1
