# As of February 2025 we require CMake 3.22.0
cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)


# As of cMake 3.27, find_package() will look for both case-sensitive and all-uppercase versions
# of the package. This seems to affect FLANN as included by Pixi
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

# FindPythonInterp and FindPythonLibs modules are deprecated, but are still in use by
# all versions of Shiboken2. This policy must be kept at OLD until Shiboken2 is no longer
# supported
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 OLD)
endif()

# The exec_program command was deprecated in cMake 3.0, and policy CMP0153 was added in cMake
# 3.28 to control whether this gives a warning (the OLD behavior) or a fatal error (NEW)
if(POLICY CMP0153)
    cmake_policy(SET CMP0153 NEW)
endif()

# Suppress 'FindBoost module is removed' warning. Will use BoostConfig.cmake instead
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# As of cMake 3.31 add_custom_command() rejects invalid arguments
if(POLICY CMP0175)
    cmake_policy(SET CMP0175 NEW)
endif()

# Added in cMake 3.31, enforce normalization of all DESTINATION paths given to install()
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()


option(FREECAD_USE_CCACHE "Auto detect and use ccache during compilation" ON)

if(FREECAD_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)  #This check should occur before project()
    if(CCACHE_PROGRAM)
        message(STATUS "Using ccache found at: " ${CCACHE_PROGRAM})
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    endif()
endif()

project(FreeCAD)

set(PACKAGE_VERSION_MAJOR "1")
set(PACKAGE_VERSION_MINOR "2")
set(PACKAGE_VERSION_PATCH "0") # number of patch release (e.g. "4" for the 0.18.4 release)
set(PACKAGE_VERSION_SUFFIX "dev") # either "dev" for development snapshot or "" (empty string)
set(PACKAGE_BUILD_VERSION "0") # used when the same FreeCAD version will be re-released (for example using an updated LibPack)

set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(PACKAGE_STRING "${PROJECT_NAME} ${PACKAGE_VERSION}")

# include local modules
include(CheckCXXCompilerFlag)
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
SetupDoxygen()
SetupLibFmt()
SetupYamlCpp()
SetupZipIos()
find_package(ICU REQUIRED COMPONENTS uc i18n)
if(NOT FREECAD_LIBPACK_USE OR FREECAD_LIBPACK_CHECKFILE_CLBUNDLER OR FREECAD_LIBPACK_CHECKFILE_VERSION)
    SetupPython()
    SetupPCL()
    SetupPybind11()
    SetupXercesC()
    find_package(ZLIB REQUIRED)
    find_package(PyCXX REQUIRED)
    SetupOpenCasCade()
    if(BUILD_GUI)
        # Do this before the check for SMESH because it depends on vtk
        # that may have its own OpenGL check but possibly fails and leaves
        # OPENGL_gl_LIBRARY empty that results into linker errors
        SetupOpenGL()
    endif(BUILD_GUI)
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
        SetupCoin3D()
        SetupPivy()
        SetupSpaceball()
        SetupShibokenAndPyside()
        SetupMatplotlib()
    endif(BUILD_GUI)

    # SetupCoin3D can overwrite find_package(Boost) output so keep this after.
    SetupBoost()
endif()

if(BUILD_VR)
    find_package(Rift)
endif(BUILD_VR)

SetLibraryVersions()
SetGlobalCompilerAndLinkerSettings()

add_subdirectory(src)
add_subdirectory(data)

BuildAndInstallDesignerPlugin()

CreatePackagingTargets()

if(MSVC AND FREECAD_LIBPACK_USE AND LIBPACK_FOUND)
    CopyLibpackDirectories()
endif()

if (ENABLE_DEVELOPER_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

PrintFinalReport()

message("\n=================================================\n"
        "Now run 'cmake --build ${CMAKE_BINARY_DIR}' to build ${PROJECT_NAME}\n"
        "=================================================\n")
