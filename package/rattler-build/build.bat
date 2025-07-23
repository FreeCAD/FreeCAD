@echo on

@REM :: free up extra disk space, compare
@REM :: https://github.com/conda-forge/conda-smithy/issues/1949
@REM rmdir /s /q C:\hostedtoolcache\windows

@REM set "CFLAGS= "
@REM set "CXXFLAGS= -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1"
@REM set "LDFLAGS_SHARED= ucrt.lib"

set "CMAKE_GENERATOR="
set "CMAKE_GENERATOR_PLATFORM="

cmake ^
    %CMAKE_ARGS% ^
    --preset conda-windows-release ^
    -D CMAKE_C_COMPILER:STRING="%CC%" ^
    -D CMAKE_CXX_COMPILER:STRING="%CXX%" ^
    -D CMAKE_INCLUDE_PATH:FILEPATH="%LIBRARY_PREFIX%/include" ^
    -D CMAKE_INSTALL_LIBDIR:FILEPATH="%LIBRARY_PREFIX%/lib" ^
    -D CMAKE_INSTALL_PREFIX:FILEPATH="%LIBRARY_PREFIX%" ^
    -D CMAKE_LIBRARY_PATH:FILEPATH="%LIBRARY_PREFIX%/lib" ^
    -D CMAKE_PREFIX_PATH:FILEPATH="%LIBRARY_PREFIX%" ^
    -D FREECAD_USE_OCC_VARIANT="Official Version" ^
    -D INSTALL_TO_SITEPACKAGES:BOOL=ON ^
    -D OCC_INCLUDE_DIR:FILEPATH="%LIBRARY_PREFIX%/include" ^
    -D OCCT_CMAKE_FALLBACK:BOOL=OFF ^
    -D Python_EXECUTABLE:FILEPATH="%PYTHON%" ^
    -D Python3_EXECUTABLE:FILEPATH="%PYTHON%" ^
    -B build ^
    -S .
if %ERRORLEVEL% neq 0 exit 1

ninja -C build install
if %ERRORLEVEL% neq 0 exit 1

rmdir /s /q "%LIBRARY_PREFIX%\doc"
ren %LIBRARY_PREFIX%\bin\FreeCAD.exe freecad.exe
ren %LIBRARY_PREFIX%\bin\FreeCADCmd.exe freecadcmd.exe
