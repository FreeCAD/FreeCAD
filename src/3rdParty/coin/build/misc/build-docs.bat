@echo off

set type=%1
set mode=%2
set msvc=%3
set libname=%4

set doxygen=%ProgramFiles%\doxygen\bin\doxygen.exe
set htmldir=..\html

rem ************************************************************************

if exist "%doxygen%" goto doxygenexists
echo
echo   You do not seem to have have doxygen.exe installed as
echo   %doxygen%.
echo
echo   Download the windows installer from www.doxygen.org, and
echo   install it to C:\Program Files\doxygen\, or you can edit the
echo   ..\misc\build-docs.bat script file to pick up doxygen from
echo   where you have installed it.
echo
exit 1

:doxygenexists

rem ************************************************************************

if exist %htmldir%\*.* goto htmldirexists
mkdir %htmldir%
:htmldirexists

echo Running doxygen (it may take a while)...
"%doxygen%" docs\%libname%.doxygen
echo Done.

echo Copying files...
xcopy ..\..\docs\doxygen\Coin_logo.png %htmldir%\ /R /Y

explorer %htmldir%\index.html
exit 0

