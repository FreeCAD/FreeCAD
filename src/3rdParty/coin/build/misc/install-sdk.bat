@echo off

set type=%1
set mode=%2
set msvc=%3
set libname=%4

rem ************************************************************************
rem * check script arguments

if "%type%"=="dll" goto argonegiven
if "%type%"=="lib" goto argonegiven
goto argproblem
:argonegiven

if "%mode%"=="release" goto argtwogiven
if "%mode%"=="debug" goto argtwogiven
goto argproblem
:argtwogiven

if "%msvc%"=="msvc6" goto argthreegiven
if "%msvc%"=="msvc7" goto argthreegiven
if "%msvc%"=="msvc8" goto argthreegiven
if "%msvc%"=="msvc9" goto argthreegiven
if "%msvc%"=="msvc10" goto argthreegiven
goto argproblem
:argthreegiven

if "%libname%"=="coin2" goto argfourgiven
if "%libname%"=="coin3" goto argfourgiven
if "%libname%"=="coin4" goto argfourgiven
if "%libname%"=="simage1" goto argfourgiven
if "%libname%"=="smallchange1" goto argfourgiven
if "%libname%"=="simvoleon1" goto argfourgiven
if "%libname%"=="simvoleon2" goto argfourgiven
if "%libname%"=="nutsnbolts0" goto argfourgiven
if "%libname%"=="soqt1" goto argfourgiven
if "%libname%"=="sowin1" goto argfourgiven
rem goto argproblem
:argfourgiven

goto argtestdone

:argproblem
echo Error with script arguments "%1" "%2" "%3" "%4".
echo Usage:
echo   install-sdk.bat {dll,lib} {release,debug} {msvc6,msvc7,msvc8,msvc9} libname
exit

:argtestdone

rem ************************************************************************
rem * check environment variables

if not "%COINDIR%"=="" goto coindirset
echo The COINDIR environment variable must be set to point to a directory
echo to be able to perform the installation procedure.
exit

:coindirset
if exist %COINDIR%\*.* goto coindirexists
echo The COINDIR environment variable must point to an existing directory
echo to be able to perform the installation procedure.
exit

:coindirexists

rem **********************************************************************
rem * Check that build has been performed...

if "%1"=="dll" goto checkdll
goto checklib

:checkdll

if "%2"=="debug" goto checkdlldebug
goto checkdllrelease

:checkdlldebug
if exist %libname%d.dll goto checkdone
goto checkfailed

:checkdllrelease
if exist %libname%.dll goto checkdone
goto checkfailed

:checklib

if "%2"=="debug" goto checklibdebug
goto checklibrelease

:checklibdebug
if exist %libname%sd.lib goto checkdone
goto checkfailed

:checklibrelease
if exist %libname%s.lib goto checkdone
goto checkfailed

:checkfailed
echo ERROR: You do not seem to have compiled the %2-version of %libname% yet.
exit

:checkdone

rem **********************************************************************

echo Installing to %COINDIR%

rem **********************************************************************
rem * Create all the directories

if exist ..\misc\create-directories.bat goto createdirsexists
goto donecreatedirs
:createdirsexists
echo Creating necessary directories...
call ..\misc\create-directories.bat
:donecreatedirs

rem **********************************************************************
rem * Copy files

if exist ..\misc\install-headers.bat goto installheadersexists
goto doneinstallheaders
:installheadersexists
echo Installing header files...
call ..\misc\install-headers.bat %msvc%
:doneinstallheaders

if exist ..\misc\install-data.bat goto installdataexists
goto doneinstalldata
:installdataexists
echo Installing data files...
call ..\misc\install-data.bat %msvc%
:doneinstalldata

rem **********************************************************************

echo Installing binaries...

if "%1"=="dll" goto installdll
goto installlib

:installdll

if "%2"=="debug" goto installdlldebug
goto installdllrelease

:installdlldebug
xcopy %libname%d.dll %COINDIR%\bin\ /R /Y
xcopy Debug\%libname%d.pdb %COINDIR%\bin\ /R /Y
xcopy Debug\%libname%d.lib %COINDIR%\lib\ /R /Y
goto binariesdone

:installdllrelease
xcopy %libname%.dll %COINDIR%\bin\ /R /Y
xcopy Release\%libname%.lib %COINDIR%\lib\ /R /Y
goto binariesdone

:installlib

if "%2"=="debug" goto installlibdebug
goto installlibrelease

:installlibdebug
xcopy %libname%sd.lib %COINDIR%\lib\ /R /Y
goto binariesdone

:installlibrelease
xcopy %libname%s.lib %COINDIR%\lib\ /R /Y
goto binariesdone

:binariesdone
