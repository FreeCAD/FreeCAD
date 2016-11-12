project(FreeCAD_trunk)

set(PACKAGE_NAME "FreeCAD")
set(PACKAGE_VERSION_NAME "Vulcan")
set(PACKAGE_VERSION_MAJOR "0")
set(PACKAGE_VERSION_MINOR "17")
set(PACKAGE_VERSION_PATCH "6700")

set(FREECAD_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    # added in cmake 3.0
    if(POLICY CMP0050)
        cmake_policy(SET CMP0050 OLD)
    endif(POLICY CMP0050)
    if (POLICY CMP0045)
        cmake_policy(SET CMP0045 NEW)
    endif(POLICY CMP0045)
endif(COMMAND cmake_policy)

find_program(CCACHE_PROGRAM ccache)  #This check should occur before project()
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# include local  modules
include(AddFileDependencies)
include(cMake/FreeCadMacros.cmake)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cMake")


#if(CMAKE_CFG_INTDIR STREQUAL .)
    # No Debug/Release output paths
    set(DEBUG_MAIN_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
    set(RELEASE_MAIN_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
#else(CMAKE_CFG_INTDIR STREQUAL .)
#   set(DEBUG_MAIN_OUTPUT_PATH ${CMAKE_BINARY_DIR}/src/Main/Debug)
#   set(RELEASE_MAIN_OUTPUT_PATH ${CMAKE_BINARY_DIR}/src/Main/Release)
#endif(CMAKE_CFG_INTDIR STREQUAL .)

if(WIN32)
    set(PLATFORM_CP xcopy /Y /S)
    set(PLATFORM_MK mkdir)
else(WIN32)
    set(PLATFORM_CP cp)
    set(PLATFORM_MK mkdir -p)
endif(WIN32)

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_COMPILER_IS_CLANGXX TRUE)
endif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")

# ================================================================================

# Allow developers to use Boost < 1.48
if(NOT ${BOOST_MIN_VERSION})
   set(BOOST_MIN_VERSION 1.48)
endif()

# For older cmake versions the variable 'CMAKE_CXX_COMPILER_VERSION' is missing
IF(CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_CXX_COMPILER_VERSION)
    EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                    OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION)
ENDIF(CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_CXX_COMPILER_VERSION)

# Enabled C++11 for Freecad 0.17 and later
IF(FREECAD_VERSION VERSION_GREATER 0.16)
    OPTION(BUILD_ENABLE_CXX11 "Enable C++11 support." ON)
    IF(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7)
        MESSAGE(FATAL_ERROR "FreeCAD 0.17 and later requires C++11.  G++ must be 4.7 or later, the used version is ${CMAKE_CXX_COMPILER_VERSION}")
    ELSEIF(CMAKE_COMPILER_IS_CLANGXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        MESSAGE(FATAL_ERROR "FreeCAD 0.17 and later requires C++11.  Clang must be 3.3 or later, the used version is ${CMAKE_CXX_COMPILER_VERSION}")
    ENDIF()
ENDIF(FREECAD_VERSION VERSION_GREATER 0.16)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    include(cMake/ConfigureChecks.cmake)
    configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    add_definitions(-DHAVE_CONFIG_H)
    if(BUILD_ENABLE_CXX11)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif(BUILD_ENABLE_CXX11)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-write-strings ${CMAKE_CXX_FLAGS}")
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

    # get linker errors as soon as possible and not at runtime e.g. for modules
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-undefined,error")
    elseif(UNIX)
        set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
    endif()
endif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)

# ================================================================================
# Output directories for install target

if(WIN32)
    SET(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Installation root directory")
else(WIN32)
    SET(CMAKE_INSTALL_PREFIX "/usr/lib${LIB_SUFFIX}/freecad" CACHE PATH "Installation root directory")
endif(WIN32)

SET(CMAKE_INSTALL_DATADIR data CACHE PATH "Output directory for data and resource files")
SET(CMAKE_INSTALL_INCLUDEDIR include CACHE PATH "Output directory for header files")
SET(CMAKE_INSTALL_DOCDIR doc CACHE PATH "Output directory for documentation and license files")
# Don't set it without manual adaption of LibDir variable in src/App/FreeCADInit.py
SET(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

if(NOT WIN32)
    if(NOT IS_ABSOLUTE ${CMAKE_INSTALL_LIBDIR})
	    SET(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    endif(NOT IS_ABSOLUTE ${CMAKE_INSTALL_LIBDIR})
endif(NOT WIN32)

SET(PYCXX_INCLUDE_DIR
    "${CMAKE_SOURCE_DIR}/src" CACHE PATH
    "Path to the directory containing PyCXX's CXX/Config.hxx include file")
SET(PYCXX_SOURCE_DIR
    "${CMAKE_SOURCE_DIR}/src/CXX" CACHE PATH
    "Path to the directory containing PyCXX's cxxextensions.c source file")

# used as compiler defines
SET(RESOURCEDIR "${CMAKE_INSTALL_DATADIR}")
SET(DOCDIR "${CMAKE_INSTALL_DOCDIR}")

MESSAGE(STATUS "prefix: ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "datadir: ${CMAKE_INSTALL_DATADIR}")
MESSAGE(STATUS "docdir: ${CMAKE_INSTALL_DOCDIR}")
MESSAGE(STATUS "includedir: ${CMAKE_INSTALL_INCLUDEDIR}")
MESSAGE(STATUS "libdir: ${CMAKE_INSTALL_LIBDIR}")

# ==============================================================================
# == Win32 is default behaviour use the LibPack copied in Source tree ==========
if(MSVC)
    OPTION(FREECAD_RELEASE_PDB "Create PDB file for Release version." OFF)
    OPTION(FREECAD_LIBPACK_USE "Use the LibPack to Build FreeCAD (only Win32 so far)." ON)
    OPTION(FREECAD_LIBPACK_USEPYSIDE "Use PySide in LibPack rather to PyQt and Swig." ON)
    set(FREECAD_LIBPACK_DIR ${CMAKE_SOURCE_DIR} CACHE PATH  "Directory of the FreeCAD LibPack")
else(MSVC)
    OPTION(FREECAD_LIBPACK_USE "Use the LibPack to Build FreeCAD (only Win32 so far)." OFF)
    set(FREECAD_LIBPACK_DIR ""  CACHE PATH  "Directory of the FreeCAD LibPack")
endif(MSVC)

# ==============================================================================
# == All the options for the build process    ============

OPTION(BUILD_GUI "Build FreeCAD Gui. Otherwise you have only the command line and the Python import module." ON)
OPTION(FREECAD_MAINTAINERS_BUILD "Build FreeCAD for Maintainers, with Docu and 3rd party libs. On Windows the Installer is build." OFF)
OPTION(FREECAD_USE_EXTERNAL_ZIPIOS "Use system installed zipios++ instead of the bundled." OFF)
OPTION(FREECAD_USE_EXTERNAL_SMESH "Use system installed smesh instead of the bundled." OFF)
OPTION(FREECAD_USE_EXTERNAL_KDL "Use system installed orocos-kdl instead of the bundled." OFF)
OPTION(FREECAD_USE_FREETYPE "Builds the features using FreeType libs" ON)
OPTION(FREECAD_BUILD_DEBIAN "Prepare for a build of a Debian package" OFF)

if(APPLE)
    OPTION(FREECAD_CREATE_MAC_APP "Create app bundle on install" OFF)

    if(FREECAD_CREATE_MAC_APP)
        install(
            DIRECTORY ${CMAKE_SOURCE_DIR}/src/MacAppBundle/FreeCAD.app/
            DESTINATION ${CMAKE_INSTALL_PREFIX}/${PACKAGE_NAME}.app
        )

        # It should be safe to assume we've got sed on OSX...
        install(CODE "
            execute_process(COMMAND
                sed -i \"\" -e s/VERSION_STRING_FROM_CMAKE/${PACKAGE_VERSION}/
                -e s/NAME_STRING_FROM_CMAKE/${PACKAGE_NAME}/
                ${CMAKE_INSTALL_PREFIX}/${PACKAGE_NAME}.app/Contents/Info.plist)
               ")

        set(CMAKE_INSTALL_PREFIX
            ${CMAKE_INSTALL_PREFIX}/${PACKAGE_NAME}.app/Contents)
        set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
    endif(FREECAD_CREATE_MAC_APP)
endif(APPLE)

OPTION(BUILD_FEM "Build the FreeCAD FEM module" ON)
OPTION(BUILD_SANDBOX "Build the FreeCAD Sandbox module which is only for testing purposes" OFF)
OPTION(BUILD_TEMPLATE "Build the FreeCAD template module which is only for testing purposes" OFF)
OPTION(BUILD_ARCH "Build the FreeCAD Architecture module" ON)
OPTION(BUILD_ASSEMBLY "Build the FreeCAD Assembly module" OFF)
OPTION(BUILD_COMPLETE "Build the FreeCAD complete module" ON)
OPTION(BUILD_DRAFT "Build the FreeCAD draft module" ON)
OPTION(BUILD_DRAWING "Build the FreeCAD drawing module" ON)
OPTION(BUILD_IDF "Build the FreeCAD idf module" ON)
OPTION(BUILD_IMAGE "Build the FreeCAD image module" ON)
OPTION(BUILD_IMPORT "Build the FreeCAD import module" ON)
OPTION(BUILD_INSPECTION "Build the FreeCAD inspection module" ON)
OPTION(BUILD_JTREADER "Build the FreeCAD jt reader module" OFF)
OPTION(BUILD_MATERIAL "Build the FreeCAD material module" ON)
OPTION(BUILD_MESH "Build the FreeCAD mesh module" ON)
OPTION(BUILD_MESH_PART "Build the FreeCAD mesh part module" ON)
OPTION(BUILD_OPENSCAD "Build the FreeCAD openscad module" ON)
OPTION(BUILD_PART "Build the FreeCAD part module" ON)
OPTION(BUILD_PART_DESIGN "Build the FreeCAD part design module" ON)
OPTION(BUILD_PATH "Build the FreeCAD path module" ON)
OPTION(BUILD_PLOT "Build the FreeCAD plot module" ON)
OPTION(BUILD_POINTS "Build the FreeCAD points module" ON)
OPTION(BUILD_RAYTRACING "Build the FreeCAD ray tracing module" ON)
OPTION(BUILD_REVERSEENGINEERING "Build the FreeCAD reverse engineering module" ON)
OPTION(BUILD_ROBOT "Build the FreeCAD robot module" ON)
OPTION(BUILD_SHIP "Build the FreeCAD ship module" ON)
OPTION(BUILD_SHOW "Build the FreeCAD Show module (helper module for visibility automation)" ON)
OPTION(BUILD_SKETCHER "Build the FreeCAD sketcher module" ON)
OPTION(BUILD_SPREADSHEET "Build the FreeCAD spreadsheet module" ON)
OPTION(BUILD_START "Build the FreeCAD start module" ON)
OPTION(BUILD_TEST "Build the FreeCAD test module" ON)
OPTION(BUILD_TECHDRAW "Build the FreeCAD Technical Drawing module" ON)
OPTION(BUILD_WEB "Build the FreeCAD web module" ON)
OPTION(BUILD_VR "Build the FreeCAD Oculus Rift support (need Oculus SDK 4.x or higher)" OFF)

if(MSVC)
    OPTION(BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" ON)
    OPTION(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." ON)
elseif(APPLE)
    find_library(3DCONNEXIONCLIENT_FRAMEWORK 3DconnexionClient)
    if(IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK})
        OPTION(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." ON)
    else(IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK})
        OPTION(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." OFF)
    endif(IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK})
else(MSVC)
    set(FREECAD_USE_3DCONNEXION OFF)
endif(MSVC)
if(NOT MSVC)
    OPTION(BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" OFF)
    OPTION(FREECAD_USE_PCL "Build the features that use PCL libs" OFF)
endif(NOT MSVC)

# if this is set override some options
if (FREECAD_BUILD_DEBIAN)
    set(FREECAD_USE_EXTERNAL_ZIPIOS ON)
    # A Debian package for SMESH doesn't exist
    #set(FREECAD_USE_EXTERNAL_SMESH ON)
endif (FREECAD_BUILD_DEBIAN)
if(BUILD_FEM)
    set(BUILD_SMESH ON)
endif()

# ==============================================================================
#inter-module dependencies

# Takes a dependent module followed by a variable-length list of prerequisite
# modules.  Warns if any of the prerequisite modules are disabled.
function(REQUIRES_MODS dependent)
    if(${dependent})
        foreach(prerequisite ${ARGN})
            if(NOT ${prerequisite})
                message(WARNING
                        "${dependent} requires ${ARGN} each be ON, but "
                        "${prerequisite} is \"${${prerequisite}}\"")
            endif(NOT ${prerequisite})
        endforeach()
    endif(${dependent})
endfunction(REQUIRES_MODS)

REQUIRES_MODS(BUILD_COMPLETE           BUILD_MESH BUILD_POINTS BUILD_DRAWING
                                       BUILD_RAYTRACING BUILD_PART_DESIGN
                                       BUILD_IMAGE BUILD_DRAFT)
REQUIRES_MODS(BUILD_ARCH               BUILD_PART BUILD_MESH BUILD_DRAFT)
REQUIRES_MODS(BUILD_DRAFT              BUILD_SKETCHER)
REQUIRES_MODS(BUILD_DRAWING            BUILD_PART BUILD_SPREADSHEET)
REQUIRES_MODS(BUILD_FEM                BUILD_PART)
REQUIRES_MODS(BUILD_IDF                BUILD_PART)
REQUIRES_MODS(BUILD_IMPORT             BUILD_PART)
REQUIRES_MODS(BUILD_INSPECTION         BUILD_MESH BUILD_POINTS BUILD_PART)
REQUIRES_MODS(BUILD_JTREADER           BUILD_MESH)
REQUIRES_MODS(BUILD_MESH_PART          BUILD_PART BUILD_MESH BUILD_SMESH)
REQUIRES_MODS(BUILD_OPENSCAD           BUILD_MESH_PART BUILD_DRAFT)
REQUIRES_MODS(BUILD_PART_DESIGN        BUILD_SKETCHER)
REQUIRES_MODS(BUILD_PATH               BUILD_PART BUILD_ROBOT)
REQUIRES_MODS(BUILD_RAYTRACING         BUILD_PART)
REQUIRES_MODS(BUILD_REVERSEENGINEERING BUILD_PART BUILD_MESH)
REQUIRES_MODS(BUILD_ROBOT              BUILD_PART)
REQUIRES_MODS(BUILD_SANDBOX            BUILD_PART BUILD_MESH)
REQUIRES_MODS(BUILD_SHIP               BUILD_PART BUILD_PLOT BUILD_IMAGE)
REQUIRES_MODS(BUILD_SKETCHER           BUILD_PART)
REQUIRES_MODS(BUILD_SPREADSHEET        BUILD_DRAFT)
REQUIRES_MODS(BUILD_START              BUILD_WEB)
REQUIRES_MODS(BUILD_TECHDRAW           BUILD_PART BUILD_SPREADSHEET)

# ==============================================================================

if(FREECAD_LIBPACK_USE)

    # checking for a unique file in LibPack location to make sure the right version of the LibPack is there
    find_file(FREECAD_LIBPACK_CHECKFILE6X boost_program_options-vc80-mt-gd.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKFILE7X boost_program_options-vc90-mt-gd-1_39.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKFILE8X boost_program_options-vc90-mt-gd-1_48.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKFILE9X boost_program_options-vc90-mt-gd-1_54.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKFILE10X boost_program_options-vc110-mt-1_55.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKCUSTOM boost_program_options-vc90-mt-gd-1_41.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER MANIFEST.db ${FREECAD_LIBPACK_DIR})

    # don't show them in the GUI
    set(FREECAD_LIBPACK_CHECKFILE6X "${FREECAD_LIBPACK_CHECKFILE6X}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKFILE7X "${FREECAD_LIBPACK_CHECKFILE7X}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKFILE8X "${FREECAD_LIBPACK_CHECKFILE8X}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKFILE9X "${FREECAD_LIBPACK_CHECKFILE9X}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKFILE10X "${FREECAD_LIBPACK_CHECKFILE10X}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKCUSTOM "${FREECAD_LIBPACK_CHECKCUSTOM}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER "${FREECAD_LIBPACK_CHECKFILE_CLBUNDLER}" CACHE INTERNAL "Find libpack")

    IF(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
        set(FREECAD_LIBPACK_VERSION "CLbundler" CACHE STRING "Displays if the libpack has been found" FORCE)
        include(cMake/UseLibPackCLbundler.cmake)
    ELSEIF(FREECAD_LIBPACK_CHECKFILE6X)
        set(FREECAD_LIBPACK_VERSION "6.x" CACHE STRING "Displays if the libpack has been found" FORCE)
        include(cMake/UseLibPack6x.cmake)
    ELSEIF(FREECAD_LIBPACK_CHECKFILE7X)
        set(FREECAD_LIBPACK_VERSION "7.x" CACHE STRING "Displays if the libpack has been found" FORCE)
        include(cMake/UseLibPack7x.cmake)
    ELSEIF(FREECAD_LIBPACK_CHECKFILE8X)
        set(FREECAD_LIBPACK_VERSION "8.x" CACHE STRING "Displays if the libpack has been found" FORCE)
        include(cMake/UseLibPack8x.cmake)
        set(SWIG_EXECUTABLE ${FREECAD_LIBPACK_DIR}/tools/swigwin-1.3.40/swig.exe CACHE STRING "Swig" FORCE)
    ELSEIF(FREECAD_LIBPACK_CHECKFILE9X)
        set(FREECAD_LIBPACK_VERSION "9.x" CACHE STRING "Displays if the libpack has been found" FORCE)
        include(cMake/UseLibPack9x.cmake)
        set(SWIG_EXECUTABLE ${FREECAD_LIBPACK_DIR}/tools/swigwin-2.0.11/swig.exe CACHE STRING "Swig" FORCE)
        set(FREECAD_LIBPACK_PYSIDEUIC_REL   "${FREECAD_LIBPACK_DIR}/pyside-tools/Lib/site-packages")
        file(GLOB FREECAD_LIBPACK_PIVY_COIN "${FREECAD_LIBPACK_DIR}/pivy/*.*")
        file(GLOB FREECAD_LIBPACK_SHIBOKEN  "${FREECAD_LIBPACK_DIR}/shiboken-1.2.1/lib/site-packages/*.pyd")
        file(GLOB FREECAD_LIBPACK_PYSIDE    "${FREECAD_LIBPACK_DIR}/pyside/lib/site-packages/PySide/*.py*")
        file(GLOB_RECURSE FREECAD_LIBPACK_PYSIDEUIC RELATIVE "${FREECAD_LIBPACK_PYSIDEUIC_REL}" "${FREECAD_LIBPACK_PYSIDEUIC_REL}/pysideuic/*.py")
        file(GLOB FREECAD_LIBPACK_PYTHON    "${FREECAD_LIBPACK_DIR}/bin/*.py*")
    ELSEIF(FREECAD_LIBPACK_CHECKFILE10X)
        set(FREECAD_LIBPACK_VERSION "10.x" CACHE STRING "Displays if the libpack has been found" FORCE)
        include(cMake/UseLibPack10x.cmake)
        set(SWIG_EXECUTABLE ${FREECAD_LIBPACK_DIR}/tools/swigwin-3.0.2/swig.exe CACHE STRING "Swig" FORCE)
    ELSEIF(FREECAD_LIBPACK_CHECKCUSTOM)
        set(FREECAD_LIBPACK_VERSION "Custom" CACHE STRING "Displays if the libpack has been found" FORCE)
        include(cMake/UseLibPackCustom.cmake)
    ELSE(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
        set(FREECAD_LIBPACK_VERSION "NOTFOUND" CACHE STRING "Displays if the libpack has been found" FORCE)
        message(SEND_ERROR  "Could not find libpack in specified location:" ${FREECAD_LIBPACK_DIR})
    ENDIF(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)

# -------------------------------- PyCXX --------------------------------

    find_package(PyCXX REQUIRED)

# -------------------------------- Swig ----------------------------------

    find_package(SWIG)

    IF(NOT SWIG_FOUND)
        MESSAGE("==================================================\n"
                "SWIG not found, don't build SWIG binding for pivy.\n"
                "==================================================\n")
    ENDIF(NOT SWIG_FOUND)

# -------------------------------- Shiboken/PySide ------------------------

    find_package(PySideTools REQUIRED) # Pyside utilities (pyside-uic & pyside-rcc)

# -------------------------------- Salome SMESH --------------------------

    if(NOT FREECAD_USE_EXTERNAL_SMESH)
        set(SMESH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/salomesmesh/inc)
    endif()

endif(FREECAD_LIBPACK_USE)

if(NOT FREECAD_LIBPACK_USE OR FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)

# ================================================================================
# == for other OSes search the packages ==========================================


# -------------------------------- Python --------------------------------

#http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=677598
# Acceptable versions of Python
set(Python_ADDITIONAL_VERSIONS "2.5" "2.6" "2.7" "2.8" "2.9")

# For building on OS X
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    # If the user doesn't tell us which package manager they're using
    if(NOT DEFINED MACPORTS_PREFIX AND NOT DEFINED HOMEBREW_PREFIX)

        # Try to find MacPorts path
        find_program(MACPORTS_EXECUTABLE port)
        if(EXISTS ${MACPORTS_EXECUTABLE})
            string(REPLACE "/bin/port" ""
                   MACPORTS_PREFIX ${MACPORTS_EXECUTABLE})
            message(STATUS "Detected MacPorts install at ${MACPORTS_PREFIX}")
        endif(EXISTS ${MACPORTS_EXECUTABLE})

        # Try to find Homebrew path
        find_program(HOMEBREW_EXECUTABLE brew)
        if(EXISTS ${HOMEBREW_EXECUTABLE})
            string(REPLACE "/bin/brew" ""
                   HOMEBREW_PREFIX ${HOMEBREW_EXECUTABLE})
            message(STATUS "Detected Homebrew install at ${HOMEBREW_PREFIX}")
        endif()

    endif(NOT DEFINED MACPORTS_PREFIX AND NOT DEFINED HOMEBREW_PREFIX)

    # In case someone tries to shoot themselves in the foot
    if(DEFINED MACPORTS_PREFIX AND DEFINED HOMEBREW_PREFIX)
        message(SEND_ERROR
                "Multiple package management systems detected - ")
        message(SEND_ERROR
                "define either MACPORTS_PREFIX or HOMEBREW_PREFIX")

    # No package manager
    elseif(NOT DEFINED MACPORTS_PREFIX AND NOT DEFINED HOMEBREW_PREFIX)
        message(SEND_ERROR
                "No package manager detected - install MacPorts or Homebrew")

    # The hopefully-normal case - one package manager identified
    else(DEFINED MACPORTS_PREFIX AND DEFINED HOMEBREW_PREFIX)

        # Construct a list like python;python2.9;python2.8;...
        set(Python_ADDITIONAL_VERSIONS_REV ${Python_ADDITIONAL_VERSIONS})
        list(REVERSE Python_ADDITIONAL_VERSIONS_REV)
        set(_PYTHON_NAMES "python")
        foreach(_PYTHON_VERSION IN LISTS Python_ADDITIONAL_VERSIONS_REV)
            list(APPEND _PYTHON_NAMES "python${_PYTHON_VERSION}")
        endforeach(_PYTHON_VERSION)

        # Find python in the package management systems, using names in that
        # list in decreasing priority.  Note that a manually specified
        # PYTHON_EXECUTABLE still has prescedence over this.
        find_program(PYTHON_EXECUTABLE
                     NAMES ${_PYTHON_NAMES}
                     PATHS ${MACPORTS_PREFIX} ${HOMEBREW_PREFIX}
                     PATH_SUFFIXES /bin
                     NO_DEFAULT_PATH)

    endif(DEFINED MACPORTS_PREFIX AND DEFINED HOMEBREW_PREFIX)

    # Warn user if we still only have the system Python
    string(FIND ${PYTHON_EXECUTABLE} "/usr/bin/python" _FIND_SYS_PYTHON)
    if(_FIND_SYS_PYTHON EQUAL 0)
        message(SEND_ERROR
                "Only found the stock Python, that's probably bad.")
    endif(_FIND_SYS_PYTHON EQUAL 0)

    # Ask Python to tell us it's include directory, if nobody else has
    if(NOT DEFINED PYTHON_INCLUDE_DIR)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
         "from distutils.sysconfig import get_python_inc;print get_python_inc()"
                        OUTPUT_VARIABLE PYTHON_INCLUDE_DIR
                        RESULT_VARIABLE PYTHON_INCLUDE_DIR_RESULT
                        ERROR_QUIET)
        if(NOT PYTHON_INCLUDE_DIR_RESULT MATCHES 0)
            message(SEND_ERROR "Failed to determine PYTHON_INCLUDE_DIR")
        endif(NOT PYTHON_INCLUDE_DIR_RESULT MATCHES 0)
    endif(NOT DEFINED PYTHON_INCLUDE_DIR)

    # Similar for the Python library - there must be an easier way...
    if(NOT DEFINED PYTHON_LIBRARY)
        # Get the library path
        execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c
      "from distutils import sysconfig;print sysconfig.get_config_var('LIBDIR')"
                        OUTPUT_VARIABLE PYTHON_LIBRARY_DIR
                        RESULT_VARIABLE PYTHON_LIBRARY_DIR_RESULT
                        ERROR_QUIET)
        string(STRIP ${PYTHON_LIBRARY_DIR} PYTHON_LIBRARY_DIR)
        if(NOT PYTHON_LIBRARY_DIR_RESULT MATCHES 0)
            message(SEND_ERROR "Failed to determine PYTHON_LIBRARY")
        endif(NOT PYTHON_LIBRARY_DIR_RESULT MATCHES 0)

        # Get library filename - might not be safe to assume .dylib extension?
        execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c
                 "import sys;print 'libpython%d.%d.dylib'%sys.version_info[0:2]"
                        OUTPUT_VARIABLE PYTHON_LIBRARY_FILE
                        RESULT_VARIABLE PYTHON_LIBRARY_FILE_RESULT
                        ERROR_QUIET)
        string(STRIP ${PYTHON_LIBRARY_FILE} PYTHON_LIBRARY_FILE)
        if(NOT PYTHON_LIBRARY_FILE_RESULT MATCHES 0)
            message(SEND_ERROR "Failed to determine PYTHON_LIBRARY")
        endif(NOT PYTHON_LIBRARY_FILE_RESULT MATCHES 0)

        set(PYTHON_LIBRARY "${PYTHON_LIBRARY_DIR}/${PYTHON_LIBRARY_FILE}")

    else(NOT DEFINED PYTHON_LIBRARY)
        # Used on MacPorts systems for finding Shiboken and PySide
        # TODO: When we start requiring minimum CMake version above
        # 2.8.11, change PATH below to DIRECTORY
        get_filename_component(PYTHON_LIBRARY_DIR ${PYTHON_LIBRARY} PATH)
    endif(NOT DEFINED PYTHON_LIBRARY)


endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    find_package(PythonInterp REQUIRED)
    set(Python_ADDITIONAL_VERSIONS ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
    IF (NOT DEFINED PYTHON_VERSION_STRING)
        find_package(PythonLibs REQUIRED)
    ELSE (NOT DEFINED PYTHON_VERSION_STRING)
        find_package(PythonLibs ${PYTHON_VERSION_STRING} EXACT)
    ENDIF(NOT DEFINED PYTHON_VERSION_STRING)


    IF(NOT PYTHONLIBS_FOUND)
        MESSAGE(FATAL_ERROR "=================================\n"
                            "Python not found, install Python!\n"
                            "=================================\n")
    ENDIF(NOT PYTHONLIBS_FOUND)

# -------------------------------- pcl ----------------------------------
    #PCL needs to be found before boost because the PCLConfig also calls find_package(Boost ...),
    #but with different components
    if(FREECAD_USE_PCL)
        find_package(PCL REQUIRED COMPONENTS common kdtree features surface io filters segmentation sample_consensus)
    endif(FREECAD_USE_PCL)

# -------------------------------- Boost --------------------------------

    SET(_boost_TEST_VERSIONS ${Boost_ADDITIONAL_VERSIONS})

    find_package(Boost ${BOOST_MIN_VERSION}
        COMPONENTS filesystem program_options regex signals system thread REQUIRED)

    IF(NOT Boost_FOUND)
        MESSAGE(FATAL_ERROR "========================================\n"
                            "boost not found, install the components:\n"
                            " * filesystem\n"
                            " * program_options\n"
                            " * regex\n"
                            " * signals\n"
                            " * system\n"
                            " * thread\n"
                            "========================================\n")
    ENDIF(NOT Boost_FOUND)

# -------------------------------- XercesC --------------------------------

    find_package(XercesC REQUIRED)
    IF(NOT XercesC_FOUND)
        MESSAGE(FATAL_ERROR "==================\n"
                            "XercesC not found.\n"
                            "==================\n")
    ENDIF(NOT XercesC_FOUND)

# -------------------------------- ZLIB --------------------------------

    find_package(ZLIB REQUIRED)

# -------------------------------- PyCXX --------------------------------

    find_package(PyCXX REQUIRED)

# -------------------------------- OpenCasCade --------------------------------

    find_package(OpenCasCade)
    IF(NOT OCC_FOUND)
        MESSAGE(FATAL_ERROR "================================================================\n"
                            "Neither OpenCASCADE Community Edition nor OpenCASCADE was found!\n"
                            "================================================================\n")
    ENDIF(NOT OCC_FOUND)

# -------------------------------- Salome SMESH --------------------------

    # Salome SMESH sources are under src/3rdParty now
    IF(BUILD_SMESH)
        #if we use smesh we definitly also need vtk, no matter of external or internal smesh
        set (VTK_COMPONENTS
            vtkCommonCore
            vtkCommonDataModel
            vtkFiltersVerdict
            vtkIOXML
            vtkFiltersCore
            vtkFiltersGeneral
            vtkIOLegacy
            vtkFiltersExtraction
            vtkFiltersSources
            vtkFiltersGeometry
        )
        # On Linux vtkParallelMPI package is required
        if (UNIX AND NOT APPLE)
            set (VTK_COMPONENTS
                ${VTK_COMPONENTS}
                vtkIOMPIParallel
                vtkParallelMPI)
        endif()

        # For Debian testing you may have to only allow a certain list of components:
        # http://forum.freecadweb.org/viewtopic.php?f=4&t=15646&start=10#p141693
        #
        #find_package(VTK COMPONENTS ${VTK_COMPONENTS} REQUIRED NO_MODULE) #don't check VERSION 6 as this would exclude version 7
        find_package(VTK REQUIRED NO_MODULE) #don't check VERSION 6 as this would exclude version 7

        set(BUILD_FEM_VTK ON)
        if(${VTK_MAJOR_VERSION} LESS 6)
            message( FATAL_ERROR "Found VTK version is <6, this is not compatible" )
        endif()
        if(${VTK_MAJOR_VERSION} EQUAL 6)
            if(${VTK_MINOR_VERSION} LESS 2)
                set(VTK_OPTIONS -DVTK_NO_QUAD_POLY)
            endif()
            if(${VTK_MINOR_VERSION} EQUAL 0)
                message(WARNING "VTK equal to 6.0 cannot be used with c++11, FEM postprocessing is disabled")
                set(BUILD_FEM_VTK OFF)
            endif()
        endif()

        if(NOT FREECAD_USE_EXTERNAL_SMESH)
            # See https://www.hdfgroup.org/HDF5/release/cmakebuild.html
            if (WIN32)
                find_package(HDF5 COMPONENTS NO_MODULE REQUIRED static)
            else()
                find_package(HDF5 REQUIRED)
            endif()
            find_package(MEDFile REQUIRED)
            set(SMESH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/salomesmesh/inc)
        else()
            find_package(SMESH)
            if(NOT SMESH_FOUND)
                message(ERROR "================\n"
                              "SMESH not found.\n"
                              "================\n")
            endif()
            include_directories(${SMESH_INCLUDE_DIR})
        endif()
        set(SMESH_FOUND TRUE)
    ENDIF(BUILD_SMESH)

# -------------------------------- Netgen --------------------------------

    if (BUILD_FEM_NETGEN)
        find_package(NETGEN)
    endif(BUILD_FEM_NETGEN)

# -------------------------------- OpenCV --------------------------------

    # not needed at the moment
    #find_package(OpenCV REQUIRED)

# -------------------------------- Swig ----------------------------------

    find_package(SWIG)

    IF(NOT SWIG_FOUND)
        MESSAGE("=====================================================\n"
                "SWIG not found, will not build SWIG binding for pivy.\n"
                "=====================================================\n")
    ENDIF(NOT SWIG_FOUND)

# -------------------------------- Eigen --------------------------------

    find_package(Eigen3)
    IF(NOT EIGEN3_FOUND)
        MESSAGE("=================\n"
                "Eigen3 not found.\n"
                "=================\n")
    ENDIF(NOT EIGEN3_FOUND)

# -------------------------------- Qt --------------------------------

# sets ${QT_LIBRARIES}

    # If using MacPorts, help the Qt4 finder.
    if(MACPORTS_PREFIX)
        if(NOT QT_QMAKE_EXECUTABLE)
            set(QT_QMAKE_EXECUTABLE ${MACPORTS_PREFIX}/libexec/qt4/bin/qmake)
        endif()
    endif()

    SET(QT_MIN_VERSION 4.5.0)
    set(QT_USE_QTNETWORK TRUE)
    set(QT_USE_QTXML TRUE)
    if(BUILD_GUI)
        set(QT_USE_QTOPENGL TRUE)
        set(QT_USE_QTSVG TRUE)
        set(QT_USE_QTUITOOLS TRUE)
        set(QT_USE_QTWEBKIT TRUE)
    endif(BUILD_GUI)

    find_package(Qt4)# REQUIRED

    include(${QT_USE_FILE})

    IF(NOT QT4_FOUND)
        MESSAGE(FATAL_ERROR "========================\n"
                            "Qt4 libraries not found.\n"
                            "========================\n")
    ENDIF(NOT QT4_FOUND)

    IF(NOT QT_QTWEBKIT_FOUND)
        MESSAGE("========================================================\n"
                "Qt Webkit not found, will not build browser integration.\n"
                "========================================================\n")
    ENDIF(NOT QT_QTWEBKIT_FOUND)


    # This is a special version of the built in macro qt4_wrap_cpp
    # It is required since moc'ed files are now included instead of being added to projects directly
    # It adds a reverse dependency to solve this
    # This has the unfortunate side effect that some files are always rebuilt
    # There is probably a cleaner solution than this
    macro(fc_wrap_cpp outfiles )
        # get include dirs
        QT4_GET_MOC_FLAGS(moc_flags)
        QT4_EXTRACT_OPTIONS(moc_files moc_options moc_target ${ARGN})
        # fixes bug 0000585: bug with boost 1.48
        SET(moc_options ${moc_options} -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

        foreach(it ${moc_files})
            get_filename_component(it ${it} ABSOLUTE)
            QT4_MAKE_OUTPUT_FILE(${it} moc_ cpp outfile)
            ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                COMMAND ${QT_MOC_EXECUTABLE}
                ARGS ${moc_options} ${it} -o ${outfile}
                MAIN_DEPENDENCY ${it}
            )
            set(${outfiles} ${${outfiles}} ${outfile})
            add_file_dependencies(${it} ${outfile})
        endforeach(it)
    endmacro(fc_wrap_cpp)

#--------------------FreeType-----------------------

    if(FREECAD_USE_FREETYPE)
        find_package(Freetype)
        if(NOT FREETYPE_FOUND)
            MESSAGE("===============================================================\n"
                    "FreeType2 not found. Part module will lack of makeWireString().\n"
                    "===============================================================\n")
        endif(NOT FREETYPE_FOUND)
    endif(FREECAD_USE_FREETYPE)

#---------------------------------------------------

    if(BUILD_GUI)
# -------------------------------- OpenGL --------------------------------

        find_package(OpenGL)
        include(FindPackageMessage)
        if(OPENGL_GLU_FOUND)
            find_package_message(OPENGL_GLU
                "Found OpenGLU: ${OPENGL_glu_LIBRARY}"
                "[${OPENGL_glu_LIBRARY}][${OEPNGL_INCLUDE_DIR}]")
        else(OPENGL_GLU_FOUND)
            message(FATAL_ERROR "======================\n"
                                "GLU library not found.\n"
                                "======================\n")
        endif(OPENGL_GLU_FOUND)

# -------------------------------- Coin3D --------------------------------

        find_package(Coin3D REQUIRED)
        if(NOT COIN3D_FOUND)
            MESSAGE(FATAL_ERROR "=================\n"
                                "Coin3D not found.\n"
                                "=================\n")
        endif(NOT COIN3D_FOUND)

# ------------------------------ Spaceball -------------------------------

        if (WIN32)
            #future
        else(WIN32)
            find_package(Spnav)
        endif(WIN32)

# -------------------------------- Shiboken/PySide ------------------------

        # set(PYTHON_SUFFIX -python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
        SET(PYTHON_SUFFIX -python2.7) # for shiboken
        SET(PYTHON_BASENAME -python2.7) # for PySide

        if(DEFINED MACPORTS_PREFIX)
            find_package(Shiboken REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
            find_package(PySide REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
        endif(DEFINED MACPORTS_PREFIX)

        find_package(Shiboken)# REQUIRED
        if(NOT SHIBOKEN_INCLUDE_DIR)
            MESSAGE(FATAL_ERROR "===================\n"
                                "shiboken not found.\n"
                                "===================\n")
        endif(NOT SHIBOKEN_INCLUDE_DIR)

        find_package(PySide)# REQUIRED
        if(NOT PYSIDE_INCLUDE_DIR)
            MESSAGE(FATAL_ERROR "=================\n"
                                "PySide not found.\n"
                                "=================\n")
        endif(NOT PYSIDE_INCLUDE_DIR)

        find_package(PySideTools REQUIRED) # PySide utilities (pyside-uic & pyside-rcc)

# ------------------------------ Matplotlib ------------------------------

        find_package(Matplotlib)
        IF(MATPLOTLIB_FOUND)
            message(STATUS "-- matplotlib-${MATPLOTLIB_VERSION} has been found.")
        ELSE(MATPLOTLIB_FOUND)
            message("=====================================================\n"
                    "matplotlib not found, Plot module won't be available.\n"
                    "=====================================================\n")
        ENDIF(MATPLOTLIB_FOUND)

# ------------------------------------------------------------------------


    endif(BUILD_GUI)


endif(NOT FREECAD_LIBPACK_USE OR FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)

if(BUILD_VR)
    find_package(Rift)
endif(BUILD_VR)

# copy build convenient files for M$
if(WIN32)
    if (EXISTS BuildAll.bat)
        configure_file(BuildAll.bat ${CMAKE_BINARY_DIR}/BuildAll.bat COPYONLY)
    endif (EXISTS BuildAll.bat)
    #configure_file(BuildAllNice.bat ${CMAKE_BINARY_DIR}/BuildAllNice.bat COPYONLY)
endif(WIN32)



# version information of libraries
#
if(OCC_INCLUDE_DIR AND EXISTS ${OCC_INCLUDE_DIR}/Standard_Version.hxx)
    set(HAVE_OCC_VERSION 1)
endif(OCC_INCLUDE_DIR AND EXISTS ${OCC_INCLUDE_DIR}/Standard_Version.hxx)

configure_file(LibraryVersions.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/LibraryVersions.h)

# ================================================================================
# == Global Compiler and Linker Settings =========================================

include_directories(${CMAKE_BINARY_DIR}/src
					${CMAKE_SOURCE_DIR}/src)

# check for 64-bit platform
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	MESSAGE(STATUS "Platform is 64-bit, set -D_OCC64")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_OCC64")
	add_definitions(-D_OCC64 )
ELSE(CMAKE_SIZEOF_VOID_P EQUAL 8)
	MESSAGE(STATUS "Platform is 32-bit")
ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 8)



IF(MSVC)
	# set default compiler settings
	SET (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /EHa")
	SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DFC_DEBUG /Zm128")
	# set default libs
	SET (CMAKE_C_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib SHFolder.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib winmm.lib comsupp.lib Ws2_32.lib dbghelp.lib ")
	set (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}")
	# set linker flag /nodefaultlib
	set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB")
	set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /NODEFAULTLIB")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
	IF(FREECAD_RELEASE_PDB)
		set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
		set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
	ENDIF(FREECAD_RELEASE_PDB)

	# Mark 32 bit executables large address aware so they can use > 2GB address space
	# NOTE: This setting only has an effect on machines with at least 3GB of RAM, although it sets the linker option it doesn't set the the linker switch 'Enable Large Addresses'
	IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE")
		SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
	ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
ELSE(MSVC)
	SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DFC_DEBUG")
        #MESSAGE(STATUS "DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
        MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
ENDIF(MSVC)
IF(MINGW)
	# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=12477
	# Actually '-Wno-inline-dllimport' should work to suppress warnings of the form:
	# inline function 'foo' is declared as dllimport: attribute ignored
	# But it doesn't work with MinGW gcc 4.5.0 while using '-Wno-attributes' seems to
	# do the trick.
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads -Wno-attributes")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthreads -Wno-attributes")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mthreads -Wl,--export-all-symbols")
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -mthreads -Wl,--export-all-symbols")
	# http://stackoverflow.com/questions/8375310/warning-auto-importing-has-been-activated-without-enable-auto-import-specifie
	# SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
	LINK_LIBRARIES(-lgdi32)
ENDIF(MINGW)

# force build directory to be different to source directory
#if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
#MESSAGE(SEND_ERROR "The build directory (${CMAKE_BINARY_DIR}) must be different to the source directory "
#                   "(${CMAKE_SOURCE_DIR}). Please choose another build directory!")
#elseif()
add_subdirectory(src)
add_subdirectory(data)
#endif()

# ================================================================================
# == Packaging ===================================================================
if(FREECAD_MAINTAINERS_BUILD AND NOT WIN32)
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An extensible Open Source CAx program")
	set(CPACK_PACKAGE_VENDOR "FreeCAD development team")
	if(WIN32)
		set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.Win32")
	else(WIN32)
		set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.Linux")
	endif(WIN32)
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/copying.lib")
	set(CPACK_PACKAGE_VERSION_MAJOR ${PACKAGE_VERSION_MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${PACKAGE_VERSION_MINOR})
	set(CPACK_PACKAGE_VERSION_PATCH ${PACKAGE_VERSION_PATCH})
	set(CPACK_SOURCE_PACKAGE_FILE_NAME ${PACKAGE_NAME}-${PACKAGE_VERSION})

	#for debian: 'cmake .. -DFREECAD_MAINTAINERS_BUILD=TRUE -DCPACK_GENERATOR="DEB"'
	IF( CMAKE_SIZEOF_VOID_P EQUAL 4 )
		set(PKG_ARCH i386)
	ELSE( CMAKE_SIZEOF_VOID_P EQUAL 4 )
		set(PKG_ARCH amd64)
	ENDIF( CMAKE_SIZEOF_VOID_P EQUAL 4 )
	set(CPACK_DEBIAN_PACKAGE_SECTION "science")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "python, oce | opencascade, libqtgui4, libcoin60, libxerces-c3.1, zlib1g, libboost-dev, libeigen2-dev")
	set(CPACK_PACKAGE_CONTACT "<root@localhost>")
	set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}_${PKG_ARCH}")

	if(WIN32)
		set(CPACK_GENERATOR "ZIP")
		set(CPACK_SOURCE_GENERATOR "ZIP")
	else(WIN32)
		if( CPACK_GENERATOR STREQUAL "" )
			set(CPACK_GENERATOR "TGZ")
			set(CPACK_SOURCE_GENERATOR "TGZ")
		endif()
	endif(WIN32)
	file(GLOB DOT ".*")
	file(GLOB TILD "*~")
	set(CPACK_SOURCE_IGNORE_FILES
	  "${DOT}"
	  "${TILD}"
	  ".a$"
	  ".am$"
	  ".in$"
	  ".bat$"
	  ".o$"
	  ".so$"
	  ".m4$"
	  "/_build/"
	  "/_cmake/"
	  "/.deps/"
	  "/.svn/"
	  "/CMakeFiles/"
	  "/CVS/"
	  "/autom4te.cache/"
	  "/build/"
	  "/debian/"
	  "/debug/"
	  "/docs/"
	  "/m4/"
	  "/qt-build/"
	  "/CxImage/"
	  "/WindowsInstaller/"
	  "AdditionalInfo.txt$"
	  "CMakeCache.txt$"
	  "Makefile$"
	  "\\\\.sh$"
	  "_CPack_"
	  "config.h$"
	  "config.log$"
	  "config.status$"
	  "configure$"
	  "configure.ac$"
	  "Doxyfile"
	  "html$"
	  "stamp-h1$"
	  ".swp$"
	  "tar.bz2"
	  "tar.gz"
	  "~$"
	)
	set(CPACK_SOURCE_STRIP_FILES "")
	include(CPack)

	#ADD_CUSTOM_TARGET(DIST make package_source)

endif(FREECAD_MAINTAINERS_BUILD AND NOT WIN32)
    #add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
    add_custom_target(dist-git
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/makedist.py
              --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR}
              WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_custom_target(distdfsg-git
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/makedist.py
              --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR} --dfsg
              WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
if(CMAKE_COMPILER_IS_GNUCXX OR MINGW)
    add_custom_target(distcheck-git
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/makedist.py
              --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR} --check
              WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_custom_target(distcheckdfsg-git
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/makedist.py
              --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR} --dfsg --check
              WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif(CMAKE_COMPILER_IS_GNUCXX OR MINGW)

# Print message to start build process
if (${CMAKE_BUILD_TOOL} MATCHES "make")
    MESSAGE("=======================================\n"
            "Now run 'make' to build ${PACKAGE_NAME}\n"
            "=======================================\n")
endif()
