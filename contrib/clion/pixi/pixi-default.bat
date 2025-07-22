@echo off
REM Change to the directory of this script (any subdirectory in the repository should work)
cd /d "%~dp0"

REM Activate pixi default environment
REM Write the output of pixi shell-hook to a temporary batch file and execute it
set TEMP_BATCH_FILE=%TEMP%\pixi_shell_hook_freecad.bat
pixi shell-hook > "%TEMP_BATCH_FILE%"
call "%TEMP_BATCH_FILE%"
del "%TEMP_BATCH_FILE%"