@echo off
REM SPDX-license-identifier: CC0-1.0

setlocal enabledelayedexpansion

echo "=== NSIS large-strings + advanced-logging build starting ==="
echo SRC_DIR=%SRC_DIR%
echo PREFIX=%PREFIX%

set "NSIS_SRC=%SRC_DIR%\nsis-src"
set "ZLIB_STAGE=%SRC_DIR%\zlib-w32"

if not exist "%NSIS_SRC%\SConstruct" (
    echo "ERROR: Could not find NSIS source at '%NSIS_SRC%' (SConstruct missing)."
    exit /b 1
)

if not exist "%ZLIB_STAGE%\zlib1.dll" (
    echo "ERROR: Could not find zlib at '%ZLIB_STAGE%' (zlib1.dll missing)."
    exit /b 1
)

echo NSIS source  : %NSIS_SRC%

echo.
echo "=== Building NSIS (NSIS_MAX_STRLEN=8192, NSIS_CONFIG_LOG=yes) ==="
pushd "%NSIS_SRC%"
set "ZLIB_W32=%ZLIB_STAGE%"
set "NSIS_INSTALL_PREFIX=%PREFIX%\NSIS"

echo    ZLIB_W32=%ZLIB_W32%
echo    Install prefix: %NSIS_INSTALL_PREFIX%

scons NSIS_MAX_STRLEN=8192 NSIS_CONFIG_LOG=yes ^
    "PREFIX=%NSIS_INSTALL_PREFIX%" ^
    install-compiler install-stubs

if errorlevel 1 (
    echo "ERROR: NSIS build failed."
    popd
    exit /b 1
)

popd

if not exist "%NSIS_INSTALL_PREFIX%\Bin\makensis.exe" (
    echo "ERROR: Expected makensis.exe at '%NSIS_INSTALL_PREFIX%\Bin\makensis.exe'"
    exit /b 1
)

mkdir "%PREFIX%\Scripts" 2>nul
copy "%NSIS_INSTALL_PREFIX%\Bin\makensis.exe" "%PREFIX%\Scripts\makensis.exe" /Y >nul


set "NSPROCESS_SRC=%SRC_DIR%\nsprocess-src"

if exist "%NSPROCESS_SRC%\Plugins\x86-unicode\nsProcess.dll" (
    mkdir "%NSIS_INSTALL_PREFIX%\Plugins\x86-unicode" 2>nul
    copy "%NSPROCESS_SRC%\Plugins\x86-unicode\nsProcess.dll" ^
         "%NSIS_INSTALL_PREFIX%\Plugins\x86-unicode\nsProcess.dll" /Y >nul
) else (
    echo WARNING: NsProcess.dll not found at "%NSPROCESS_SRC%\Plugins\x86-unicode\nsProcess.dll"
)


echo.
echo "=== NSIS large-strings + advanced-logging build completed successfully ==="

endlocal
exit /b 0
