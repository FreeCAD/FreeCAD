# As of January 2023 we require CMake 3.16.3
# see https://forum.freecad.org/viewtopic.php?f=10&t=72173
# for further info why
cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

# policy CMP0072 was introduced with CMake 3.11
# relates to FindOpenGL module
# and cache variables OPENGL_gl_LIBRARY, OPENGL_glu_LIBRARY
if (POLICY CMP0072)
    set(OpenGL_GL_PREFERENCE LEGACY)
endif(POLICY CMP0072)

if (BUILD_WITH_CONDA AND WIN32)
    option(FREECAD_USE_CCACHE "Auto detect and use ccache during compilation" OFF)
else()
    option(FREECAD_USE_CCACHE "Auto detect and use ccache during compilation" ON)
endif()

if(FREECAD_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)  #This check should occur before project()
    if(CCACHE_PROGRAM)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    endif()
endif()

project(FreeCAD)

set(PACKAGE_VERSION_NAME "Vulcan")
set(PACKAGE_VERSION_MAJOR "0")
set(PACKAGE_VERSION_MINOR "22")
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
if(NOT FREECAD_LIBPACK_USE OR FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
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
    SetupBoost()
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

BuildAndInstallDesignerPlugin()

CreatePackagingTargets()

if(MSVC AND FREECAD_LIBPACK_USE AND LIBPACK_FOUND)
    CopyLibpackDirectories()
endif()

if (ENABLE_DEVELOPER_TESTS)
    add_subdirectory(tests)
endif()

PrintFinalReport()

message("\n=================================================\n"
        "Now run 'cmake --build ${CMAKE_BINARY_DIR}' to build ${PROJECT_NAME}\n"
        "=================================================\n")
