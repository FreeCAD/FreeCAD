project(FreeCAD_trunk)
set(FREECAD_VERSION "0.15")

set(PACKAGE_NAME "FreeCAD")
set(PACKAGE_VERSION_MAJOR "0")
set(PACKAGE_VERSION_MINOR "15")
set(PACKAGE_VERSION_PATCH "4664")
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    # added in cmake 3.0
    if(POLICY CMP0050)
        cmake_policy(SET CMP0050 OLD)
    endif(POLICY CMP0050)
endif(COMMAND cmake_policy)

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


# ================================================================================

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_COMPILER_IS_CLANGXX TRUE)
endif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    include(cMake/ConfigureChecks.cmake)
    configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    add_definitions(-DHAVE_CONFIG_H)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-write-strings")
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
    # get linker errors as soon as possible and not at runtime e.g. for modules
    if(UNIX)
    #    SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
    endif(UNIX)
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
OPTION(FREECAD_USE_EXTERNAL_PIVY "Use system installed python-pivy instead of the bundled." OFF)
OPTION(FREECAD_USE_EXTERNAL_SMESH "Use system installed smesh instead of the bundled." OFF)
OPTION(FREECAD_BUILD_DEBIAN "Prepare for a build of a Debian package" OFF)

if(APPLE)
    OPTION(FREECAD_CREATE_MAC_APP "Create app bundle on install" OFF)

    if(FREECAD_CREATE_MAC_APP)
        install(
            DIRECTORY ${CMAKE_SOURCE_DIR}/src/MacAppBundle/FreeCAD.app
            DESTINATION ${CMAKE_INSTALL_PREFIX}
        )
        set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/FreeCAD.app/Contents)
        SET(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
    endif(FREECAD_CREATE_MAC_APP)
endif(APPLE)

OPTION(BUILD_CAM "Build the FreeCAD CAM module and the needed libs, be aware, unfinished code!" OFF)
OPTION(BUILD_FEM "Build the FreeCAD FEM module, be aware, unfinished code!" ON)
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
OPTION(BUILD_PLOT "Build the FreeCAD plot module" ON)
OPTION(BUILD_POINTS "Build the FreeCAD points module" ON)
OPTION(BUILD_RAYTRACING "Build the FreeCAD ray tracing module" ON)
OPTION(BUILD_REVERSEENGINEERING "Build the FreeCAD reverse engineering module" ON)
OPTION(BUILD_ROBOT "Build the FreeCAD robot module" ON)
OPTION(BUILD_SHIP "Build the FreeCAD ship module" ON)
OPTION(BUILD_SKETCHER "Build the FreeCAD sketcher module" ON)
OPTION(BUILD_SPREADSHEET "Build the FreeCAD spreadsheet module" ON)
OPTION(BUILD_START "Build the FreeCAD start module" ON)
OPTION(BUILD_TEST "Build the FreeCAD test module" ON)
OPTION(BUILD_WEB "Build the FreeCAD web module" ON)
OPTION(BUILD_VR "Build the FreeCAD Oculus Rift support (need Oculus SDK 4.x or higher)" OFF)

if(MSVC)
OPTION(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." ON)
OPTION(FREECAD_USE_FREETYPE "Builds the features using FreeType libs" OFF)
OPTION(BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" ON)
else(MSVC)
set(FREECAD_USE_3DCONNEXION OFF)
OPTION(FREECAD_USE_FREETYPE "Builds the features using FreeType libs" ON)
OPTION(BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" OFF)
OPTION(FREECAD_USE_PCL "Build the features that use PCL libs" OFF)
OPTION(FREECAD_USE_EXTERNAL_PIVY "Use system installed python-pivy instead of the bundled." ON)
endif(MSVC)

# if this is set override some options
if (FREECAD_BUILD_DEBIAN)
    set(FREECAD_USE_EXTERNAL_ZIPIOS ON)
    set(FREECAD_USE_EXTERNAL_PIVY ON)
    # A Debian package for SMESH doesn't exist
    #set(FREECAD_USE_EXTERNAL_SMESH ON)
endif (FREECAD_BUILD_DEBIAN)

# ==============================================================================
#inter-module dependencies

#took these from Mod/Complete/App/AppComplete.cpp. appears to be working.
if(BUILD_COMPLETE)
  set(BUILD_MESH ON)
  set(BUILD_POINTS ON)
  set(BUILD_DRAWING ON)
  set(BUILD_RAYTRACING ON)
  set(BUILD_PART_DESIGN ON)
  set(BUILD_IMAGE ON)
  set(BUILD_DRAFT ON)
endif(BUILD_COMPLETE)

#inferred from .py files. appears to be working.
if(BUILD_ARCH)
  set(BUILD_PART ON)
  set(BUILD_MESH ON)
  set(BUILD_DRAFT ON)
endif(BUILD_ARCH)

#inferred from .py files. appears to be working.
if(BUILD_OPENSCAD)
  set(BUILD_MESH_PART ON)
  set(BUILD_DRAFT ON)
endif(BUILD_OPENSCAD)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_INSPECTION)
  set(BUILD_MESH ON)
  set(BUILD_POINTS ON)
  set(BUILD_PART ON)
endif(BUILD_INSPECTION)

#inferred from .py files. appears to be working.
if(BUILD_SHIP)
  set(BUILD_PART ON)
  set(BUILD_PLOT ON)
  set(BUILD_IMAGE ON)
endif(BUILD_SHIP)

#inferred from .py files. appears to be working.
if(BUILD_SPREADSHEET)
  set(BUILD_DRAFT ON)
endif(BUILD_SPREADSHEET)

#this has been tested. appears to be working.
if(BUILD_DRAFT)
  set(BUILD_SKETCHER ON)
endif(BUILD_DRAFT)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_PART_DESIGN)
  set(BUILD_SKETCHER ON)
endif(BUILD_PART_DESIGN)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_REVERSEENGINEERING)
  set(BUILD_PART ON)
  set(BUILD_MESH ON)
endif(BUILD_REVERSEENGINEERING)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_MESH_PART)
  set(BUILD_PART ON)
  set(BUILD_MESH ON)
  set(BUILD_SMESH ON)
endif(BUILD_MESH_PART)

#inferred from cmakelists.txt. build errors. obsolete?
if(BUILD_CAM)
  set(BUILD_PART ON)
  set(BUILD_MESH ON)
endif(BUILD_CAM)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_RAYTRACING)
  set(BUILD_PART ON)
endif(BUILD_RAYTRACING)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_DRAWING)
  set(BUILD_PART ON)
endif(BUILD_DRAWING)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_ROBOT)
  set(BUILD_PART ON)
endif(BUILD_ROBOT)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_SKETCHER)
  set(BUILD_PART ON)
endif(BUILD_SKETCHER)

#inferred from .py files. built, but not sure how to test.
if(BUILD_IDF)
  set(BUILD_PART ON)
endif(BUILD_IDF)

#inferred from cmakelists.txt. built, but not sure how to test.
if(BUILD_IMPORT)
  set(BUILD_PART ON)
endif(BUILD_IMPORT)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_FEM)
  set(BUILD_PART ON)
  set(BUILD_SMESH ON)
endif(BUILD_FEM)

#inferred from cmakelists.txt. appears to be working.
if(BUILD_SANDBOX)
  set(BUILD_PART ON)
  set(BUILD_MESH ON)
endif(BUILD_SANDBOX)

#inferred from header includes. not tested.
if(BUILD_JTREADER)
  set(BUILD_MESH ON)
endif(BUILD_JTREADER)

#leaving test module for someone else.
#image has none. as far as I can tell.
#mesh has none. as far as I can tell.
#part has none. as far as I can tell.
#material has none. as far as I can tell.
#web has none. as far as I can tell.
#start has none. as far as I can tell.
#assembly has none. as far as I can tell.
#plot has none. as far as I can tell.
#points has none. as far as I can tell

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
        set(FREECAD_USE_EXTERNAL_PIVY ON CACHE BOOL "Switch off local pivy" FORCE)
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
        MESSAGE("SWIG was not found, build no SWIG binding for pivy")
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
    set(Python_ADDITIONAL_VERSIONS "2.3" "2.4" "2.5" "2.6" "2.7" "2.8" "2.9")
    find_package(PythonInterp REQUIRED)
    IF (NOT DEFINED PYTHON_VERSION_STRING)
        find_package(PythonLibs REQUIRED)
    ELSE (NOT DEFINED PYTHON_VERSION_STRING)
        find_package(PythonLibs ${PYTHON_VERSION_STRING} EXACT)
    ENDIF(NOT DEFINED PYTHON_VERSION_STRING)

    IF(NOT PYTHONLIBS_FOUND)
        MESSAGE("Python not found, install python!")
    ENDIF(NOT PYTHONLIBS_FOUND)

# -------------------------------- pcl ----------------------------------
    #PCL needs to be found before boost because the PCLConfig also calls find_package(Boost ...),
    #but with different components
    if(FREECAD_USE_PCL)
        find_package(PCL REQUIRED COMPONENTS common kdtree features surface io)
    endif(FREECAD_USE_PCL)

# -------------------------------- Boost --------------------------------

    SET( _boost_TEST_VERSIONS ${Boost_ADDITIONAL_VERSIONS} "1.41.0" "1.39.0" "1.38.0" "1.37.0"
        "1.36.1" "1.36.0" "1.35.1" "1.35.0" "1.35" "1.34.1" "1.34.0" "1.34"
        "1.33.1" "1.33.0" "1.33" )

    find_package(Boost COMPONENTS filesystem program_options regex signals system thread REQUIRED)

# -------------------------------- XercesC --------------------------------

    find_package(XercesC REQUIRED)

# -------------------------------- ZLIB --------------------------------

    find_package(ZLIB REQUIRED)

# -------------------------------- PyCXX --------------------------------

    find_package(PyCXX REQUIRED)

# -------------------------------- OpenCasCade --------------------------------

    find_package(OpenCasCade)

# -------------------------------- Salome SMESH --------------------------

    # Salome SMESH sources are under src/3rdParty now
    IF(OCC_FOUND)
    if(NOT FREECAD_USE_EXTERNAL_SMESH)
        set(SMESH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/salomesmesh/inc)
    else()
        find_package(SMESH)
        if(NOT SMESH_FOUND)
            message(ERROR "SMESH was not found.")
        endif()
        include_directories(${SMESH_INCLUDE_DIR})
    endif()
    set(SMESH_FOUND TRUE)
    ENDIF(OCC_FOUND)

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
        MESSAGE("SWIG was not found, build no SWIG binding for pivy")
    ENDIF(NOT SWIG_FOUND)

# -------------------------------- Eigen --------------------------------

    find_package(Eigen3)

# -------------------------------- ODE ----------------------------------

#    find_package(ODE)

# -------------------------------- Qt --------------------------------

# sets ${QT_LIBRARIES}

    SET(QT_MIN_VERSION 4.5.0)
    set(QT_USE_QTNETWORK TRUE)
    set(QT_USE_QTXML TRUE)
    if(BUILD_GUI)
        set(QT_USE_QTOPENGL TRUE)
        set(QT_USE_QTSVG TRUE)
        set(QT_USE_QTUITOOLS TRUE)
        set(QT_USE_QTWEBKIT TRUE)
    endif(BUILD_GUI)
    find_package(Qt4 REQUIRED)

    include(${QT_USE_FILE})

    IF(NOT QT4_FOUND)
        MESSAGE("Library qt-4.3 or above is not available, install QT or FreeCAD Gui version will not be built")
    ENDIF(NOT QT4_FOUND)

    IF(NOT QT_QTWEBKIT_FOUND)
        MESSAGE("Qt Webkit not found, will not build browser integration!")
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
            MESSAGE("FreeType2 library is not available. Part module will lack of makeWireString().")
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
                    message(FATAL_ERROR "The GLU library is required. Make sure it is installed.")
                endif(OPENGL_GLU_FOUND)

# -------------------------------- Coin3D --------------------------------

        find_package(Coin3D REQUIRED)

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
        find_package(Shiboken REQUIRED)
        find_package(PySide REQUIRED)
        find_package(PySideTools REQUIRED) # Pyside utilities (pyside-uic & pyside-rcc)

# ------------------------------ Matplotlib ------------------------------

        find_package(Matplotlib)
        IF(MATPLOTLIB_FOUND)
            message(STATUS "-- matplotlib-${MATPLOTLIB_VERSION} has been found.")
        ELSE(MATPLOTLIB_FOUND)
            message("-- matplotlib not found, Plot module won't be available!")
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
	SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
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
# 0000661: cmake build on Mac OS: dealing with dylib versus so
IF(APPLE)
	SET(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
ENDIF(APPLE)

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
	#set(CPACK_SOURCE_IGNORE_FILES
    #  ${CPACK_SOURCE_IGNORE_FILES}
	#  "/Pivy-0.5/"
	#)
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
