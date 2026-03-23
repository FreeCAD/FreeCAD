@echo off

if "%1"=="msvc6"  (
echo MSVC6
call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
) else (
if "%1"=="msvc7" (
echo MSVC7
call "%VS71COMNTOOLS%\vsvars32.bat"
) else (
if "%1"=="msvc8" (
echo MSVC8
call "%VS80COMNTOOLS%\..\..\VC\vcvarsall.bat"
) else (
if "%1"=="msvc9" (
echo MSVC9
call "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat"
) else (
if "%1"=="msvc10" (
echo MSVC10
call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat
) else (
echo ERROR Unknown compiler "%1"
goto END
)))))

bash generate-compiler.sh %1 %2

if not %ERRORLEVEL% == 0 (
  EXIT 1
)

:END
