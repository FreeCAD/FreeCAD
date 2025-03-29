set conda_env="fc_env"

robocopy ..\.pixi\envs\default\* %conda_env% /S /MT:%NUMBER_OF_PROCESSORS% > nul

%conda_env%\python ..\scripts\get_freecad_version.py
set /p freecad_version_name= <bundle_name.txt

echo **********************
echo %freecad_version_name%
echo **********************

REM remove arm binaries that fail to extract unless using latest 7zip
for /r %conda_env% %%i in (*arm*.exe) do (@echo "%%i will be removed" & @del "%%i")

set copy_dir="FreeCAD_Conda_Build"
mkdir %copy_dir%

REM Copy Conda's Python and (U)CRT to FreeCAD/bin
robocopy %conda_env%\DLLs %copy_dir%\bin\DLLs /S /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Lib %copy_dir%\bin\Lib /XD __pycache__ /S /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Scripts %copy_dir%\bin\Scripts /S /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\python*.* %copy_dir%\bin\ /XF *.pdb /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\msvc*.* %copy_dir%\bin\ /XF *.pdb /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\ucrt*.* %copy_dir%\bin\ /XF *.pdb /MT:%NUMBER_OF_PROCESSORS% > nul
REM Copy meaningful executables
robocopy %conda_env%\Library\bin %copy_dir%\bin\ ccx.exe /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\bin %copy_dir%\bin\ gmsh.exe /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\bin %copy_dir%\bin\ dot.exe /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\bin %copy_dir%\bin\ unflatten.exe /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\mingw-w64\bin * %copy_dir%\bin\ /MT:%NUMBER_OF_PROCESSORS% > nul
REM Copy Conda's QT5/plugins to FreeCAD/bin
robocopy %conda_env%\Library\plugins %copy_dir%\bin\ /S /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\resources %copy_dir%\resources /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\translations %copy_dir%\translations /MT:%NUMBER_OF_PROCESSORS% > nul
REM get all the dependency .dlls
robocopy %conda_env%\Library\bin *.dll %copy_dir%\bin /XF *.pdb /XF api*.* /MT:%NUMBER_OF_PROCESSORS% > nul
REM Copy FreeCAD build
robocopy %conda_env%\Library\bin FreeCAD* %copy_dir%\bin /XF *.pdb /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\data %copy_dir%\data /XF *.txt /S /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\Ext %copy_dir%\Ext /S /XD __pycache__ /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\lib %copy_dir%\lib /XF *.lib /XF *.prl /XF *.sh /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\Mod %copy_dir%\Mod /S /XD __pycache__ /MT:%NUMBER_OF_PROCESSORS% > nul
robocopy %conda_env%\Library\doc %copy_dir%\doc ThirdPartyLibraries.html LICENSE.html /MT:%NUMBER_OF_PROCESSORS% > nul
REM Apply Patches
rename %copy_dir%\bin\Lib\ssl.py ssl-orig.py
copy ssl-patch.py %copy_dir%\bin\Lib\ssl.py

cd %copy_dir%\..
ren %copy_dir% %freecad_version_name%
dir

REM if errorlevel1 exit 1

"%ProgramFiles%\7-Zip\7z.exe" a -t7z -mx9 -mmt=%NUMBER_OF_PROCESSORS% %freecad_version_name%.7z %freecad_version_name%\ -bb
certutil -hashfile "%freecad_version_name%.7z" SHA256 > "%freecad_version_name%.7z"-SHA256.txt
echo  %date%-%time% >>"%freecad_version_name%.7z"-SHA256.txt
