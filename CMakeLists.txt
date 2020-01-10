cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif(POLICY CMP0020)
    # added in cmake 3.0
    if(POLICY CMP0050)
        cmake_policy(SET CMP0050 NEW)
    endif(POLICY CMP0050)
    if (POLICY CMP0045)
        cmake_policy(SET CMP0045 NEW)
    endif(POLICY CMP0045)
    if (POLICY CMP0072)
        cmake_policy(SET CMP0072 OLD)
    endif(POLICY CMP0072)
endif(COMMAND cmake_policy)

find_program(CCACHE_PROGRAM ccache)  #This check should occur before project()
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

project(FreeCAD)

set(PACKAGE_VERSION_NAME "Vulcan")
set(PACKAGE_VERSION_MAJOR "0")
set(PACKAGE_VERSION_MINOR "19")
set(PACKAGE_VERSION_PATCH "16100")

set(FREECAD_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(PACKAGE_STRING "${PROJECT_NAME} ${PACKAGE_VERSION}")

# include local modules
include(AddFileDependencies)
include(cMake/FreeCadMacros.cmake)
# include helper functions/macros
add_subdirectory(cMake/FreeCAD_Helpers)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cMake")

CompilerChecksAndSetups()
ConfigureCMakeVariables()
InitializeFreeCADBuildOptions()
CheckInterModuleDependencies()
FreeCADLibpackChecks()
if(NOT FREECAD_LIBPACK_USE OR FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
    SetupPython()
    SetupPCL()
    SetupPybind11()
    SetupBoost()
    SetupXercesC()
    find_package(ZLIB REQUIRED)
    find_package(PyCXX REQUIRED)
    SetupOpenCasCade()
    SetupSalomeSMESH()
    if (BUILD_FEM_NETGEN)
        find_package(NETGEN)
    endif(BUILD_FEM_NETGEN)
    # not needed at the moment
    #find_package(OpenCV REQUIRED)
    SetupSwig()
    SetupEigen()
    # This file is included directly due to some cMake macros that are defined within.
    # If these macro definitions are embedded within another macro, it causes problems.
    include(cMake/FreeCAD_Helpers/SetupQt.cmake)
    SetupFreetype()

    if(BUILD_GUI)
        SetupOpenGL()
        SetupCoin3D()
        SetupSpaceball()
        SetupShibokenAndPyside()
        SetupMatplotlib()
    endif(BUILD_GUI)
endif(NOT FREECAD_LIBPACK_USE OR FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)

if(BUILD_VR)
    find_package(Rift)
endif(BUILD_VR)

SetLibraryVersions()
SetGlobalCompilerAndLinkerSettings()

add_subdirectory(src)
add_subdirectory(data)

CreatePackagingTargets()

PrintFinalReport()
