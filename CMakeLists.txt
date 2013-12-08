project(FreeCAD_trunk)
set(FREECAD_VERSION "0.13")

# Needed for salomesmesh
if(CMAKE_COMPILER_IS_GNUCXX)
ENABLE_LANGUAGE(Fortran)
endif(CMAKE_COMPILER_IS_GNUCXX)

set(PACKAGE_NAME "FreeCAD")
set(PACKAGE_VERSION_MAJOR "0")
set(PACKAGE_VERSION_MINOR "12")
set(PACKAGE_VERSION_PATCH "2237")
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

# include local  modules
include(AddFileDependencies)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cMake")


if(COMMAND cmake_policy)
     cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

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

if(CMAKE_COMPILER_IS_GNUCXX)
    include(cMake/ConfigureChecks.cmake)
    configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    add_definitions(-DHAVE_CONFIG_H)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-write-strings")
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
    # get linker errors as soon as possible and not at runtime e.g. for modules
    if(UNIX)
    #    SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
    endif(UNIX)
endif(CMAKE_COMPILER_IS_GNUCXX)


# ================================================================================
# Output directories for install target

if(WIN32)
    SET(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Installation root directory")
else(WIN32)
    SET(CMAKE_INSTALL_PREFIX "/usr/lib${LIB_SUFFIX}/freecad" CACHE PATH "Installation root directory")
    SET(INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif(WIN32)

SET(CMAKE_INSTALL_DATADIR data CACHE PATH "Output directory for data and resource files")
SET(CMAKE_INSTALL_INCLUDEDIR include CACHE PATH "Output directory for header files")
SET(CMAKE_INSTALL_DOCDIR doc CACHE PATH "Output directory for documentation and license files")
 
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

# ==============================================================================
# == Win32 is default behaviour use the LibPack copied in Source tree ==========
if(MSVC)
    OPTION(FREECAD_LIBPACK_USE "Use the LibPack to Build FreeCAD (only Win32 so far)." ON)
    set(FREECAD_LIBPACK_DIR ${CMAKE_SOURCE_DIR} CACHE PATH  "Directory of the FreeCAD LibPack")
else(MSVC)
    OPTION(FREECAD_LIBPACK_USE "Use the LibPack to Build FreeCAD (only Win32 so far)." OFF)
    set(FREECAD_LIBPACK_DIR ""  CACHE PATH  "Directory of the FreeCAD LibPack")
endif(MSVC)

# ==============================================================================
# == All the options for the build process    ============

OPTION(FREECAD_BUILD_GUI "Build FreeCAD Gui. Otherwise you have only the command line and the Python import module." ON)
OPTION(FREECAD_MAINTAINERS_BUILD "Build FreeCAD for Maintainers, with Docu and 3rd party libs. On Windows the Installer is build." OFF)
OPTION(FREECAD_BUILD_CAM "Build the FreeCAD CAM module and the needed libs, be aware, unfinished code!" OFF)
OPTION(FREECAD_BUILD_FEM "Build the FreeCAD FEM module, be aware, unfinished code!" ON)
OPTION(FREECAD_BUILD_SANDBOX "Build the FreeCAD Sandbox module which is only for testing purposes" OFF)
OPTION(FREECAD_BUILD_TEMPLATE "Build the FreeCAD template module which is only for testing purposes" OFF)
OPTION(FREECAD_BUILD_DEBIAN "Prepare for a build of a Debian package" OFF)
OPTION(FREECAD_USE_EXTERNAL_ZIPIOS "Use system installed zipios++ instead of the bundled." OFF)
OPTION(FREECAD_USE_EXTERNAL_PIVY "Use system installed python-pivy instead of the bundled." OFF)
if(MSVC)
OPTION(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." ON)
OPTION(FREECAD_USE_FREETYPE "Builds the features using FreeType libs" OFF)
OPTION(FREECAD_BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" ON)
else(MSVC)
set(FREECAD_USE_3DCONNEXION OFF)
OPTION(FREECAD_USE_FREETYPE "Builds the features using FreeType libs" ON)
OPTION(FREECAD_BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" OFF)
endif(MSVC)

# if this is set override some options
if (FREECAD_BUILD_DEBIAN)
    set(FREECAD_USE_EXTERNAL_ZIPIOS ON)
    set(FREECAD_USE_EXTERNAL_PIVY ON)
endif (FREECAD_BUILD_DEBIAN)

# ==============================================================================

if(FREECAD_LIBPACK_USE)

    # checking for a unique file in LibPack location to make sure the right version of the LibPack is there
    find_file(FREECAD_LIBPACK_CHECKFILE6X boost_program_options-vc80-mt-gd.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKFILE7X boost_program_options-vc90-mt-gd-1_39.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKFILE8X boost_program_options-vc90-mt-gd-1_48.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKFILE9X boost_program_options-vc90-mt-gd-1_54.lib ${FREECAD_LIBPACK_DIR}/lib )
    find_file(FREECAD_LIBPACK_CHECKCUSTOM boost_program_options-vc90-mt-gd-1_41.lib ${FREECAD_LIBPACK_DIR}/lib )

    # don't show them in the GUI
    set(FREECAD_LIBPACK_CHECKFILE6X "${FREECAD_LIBPACK_CHECKFILE6X}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKFILE7X "${FREECAD_LIBPACK_CHECKFILE7X}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKFILE8X "${FREECAD_LIBPACK_CHECKFILE8X}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKFILE9X "${FREECAD_LIBPACK_CHECKFILE9X}" CACHE INTERNAL "Find libpack")
    set(FREECAD_LIBPACK_CHECKCUSTOM "${FREECAD_LIBPACK_CHECKCUSTOM}" CACHE INTERNAL "Find libpack")

    IF(FREECAD_LIBPACK_CHECKFILE6X)
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
        file(GLOB FREECAD_LIBPACK_PIVY_COIN "${FREECAD_LIBPACK_DIR}/pivy/*.*")
        file(GLOB FREECAD_LIBPACK_PIVY_SOQT "${FREECAD_LIBPACK_DIR}/pivy/gui/*.*")
        file(GLOB FREECAD_LIBPACK_PYSIDE    "${FREECAD_LIBPACK_DIR}/pyside/lib/site-packages/PySide/*.py*")
        file(GLOB FREECAD_LIBPACK_PYTHON    "${FREECAD_LIBPACK_DIR}/bin/*.py*")
    ELSEIF(FREECAD_LIBPACK_CHECKCUSTOM)
        set(FREECAD_LIBPACK_VERSION "Custom" CACHE STRING "Displays if the libpack has been found" FORCE)
        include(cMake/UseLibPackCustom.cmake)
    ELSE(FREECAD_LIBPACK_CHECKFILE6X)
        set(FREECAD_LIBPACK_VERSION "NOTFOUND" CACHE STRING "Displays if the libpack has been found" FORCE)
        message(SEND_ERROR  "Could not find libpack in specified location:" ${FREECAD_LIBPACK_DIR})
    ENDIF(FREECAD_LIBPACK_CHECKFILE6X)

# -------------------------------- PyCXX --------------------------------

    find_package(PyCXX REQUIRED)

# -------------------------------- Swig ----------------------------------

    find_package(SWIG)

    IF(NOT SWIG_FOUND)
        MESSAGE("SWIG was not found, build no SWIG binding for pivy")
    ENDIF(NOT SWIG_FOUND)

else(FREECAD_LIBPACK_USE)


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

#first, look for OpenCASCADE Community Edition (OCE)
#if OCE is installed in a nonstandard location, add -DOCE_DIR=/path/to/dir/containing/OCEConfig.cmake
# when configuring with cmake, i.e. cmake .. -DOCE_DIR=/usr/share/cmake
    if(NOT DEFINED OCE_DIR)
        if(UNIX)
            set(OCE_DIR "/usr/local/share/cmake/")
        elseif(WIN32)
            set(OCE_DIR "c:/OCE-0.4.0/share/cmake")
        endif()
    endif()
    find_package (OCE QUIET)
    if(${OCE_FOUND})
        message("-- OpenCASCADE Community Edition has been found.")
        add_definitions (-DHAVE_CONFIG_H)
        #lib list copied from FreeCAD's FindOpenCasCade.cmake
        set(OCC_LIBRARIES
            TKFillet
            TKMesh
            TKernel
            TKG2d
            TKG3d
            TKMath
            TKIGES
            TKSTL
            TKShHealing
            TKXSBase
            TKBool
            TKBO
            TKBRep
            TKTopAlgo
            TKGeomAlgo
            TKGeomBase
            TKOffset
            TKPrim
            TKSTEP
            TKSTEPBase
            TKSTEPAttr
            TKHLR
            TKFeat
        )
        #lib list copied from FreeCAD's FindOpenCasCade.cmake
        set(OCC_OCAF_LIBRARIES
            TKCAF
            TKXCAF
            TKLCAF
            TKXDESTEP
            TKXDEIGES
            TKMeshVS
            TKAdvTools
        )
        set(OCC_INCLUDE_DIR ${OCE_INCLUDE_DIRS})
        set(OCC_FOUND ${OCE_FOUND})
    else() #look for OpenCASCADE
        find_package(OpenCasCade)
        if(NOT OCC_FOUND)
            message("Neither OpenCASCADE Community Edition nor OpenCasCade were found: will not build CAD modules!")
        else()
            message("-- OpenCASCADE include directory: ${OCC_INCLUDE_PATH}")
            message("-- OpenCASCADE shared libraries directory: ${OCC_LIB_PATH}")
        endif()
    endif()

# -------------------------------- Salome SMESH --------------------------

    # Salome SMESH sources are under src/3rdParty now
    IF(OCC_FOUND)
    set(SMESH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/salomesmesh/inc)
    set(SMESH_LIBRARIES
        StdMeshers
        #MEFISTO2
        SMESH
        DriverUNV
        SMESHDS
        DriverSTL
        DriverDAT
        Driver
        SMDS
    )
    set(SMESH_FOUND TRUE)
    ENDIF(OCC_FOUND)
    #find_package(SMESH)
    #IF(NOT SMESH_FOUND)
    #    MESSAGE("Salome SMESH was not found!")
    #ENDIF(NOT SMESH_FOUND)
    
# -------------------------------- Netgen --------------------------------

    if (FREECAD_BUILD_FEM_NETGEN)
        find_package(NETGEN)
    endif(FREECAD_BUILD_FEM_NETGEN)

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
    if(FREECAD_BUILD_GUI)
        set(QT_USE_QTOPENGL TRUE)
        set(QT_USE_QTSVG TRUE)
        set(QT_USE_QTUITOOLS TRUE)
        set(QT_USE_QTWEBKIT TRUE)
    endif(FREECAD_BUILD_GUI)
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
            QT4_CREATE_MOC_COMMAND(${it} ${outfile} "${moc_flags}" "${moc_options}" "${moc_target}")
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

    if(FREECAD_BUILD_GUI)
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
        find_package(SoQt REQUIRED)

# ------------------------------ Spaceball -------------------------------

        if (WIN32)
            #future
        else(WIN32)
            find_package(Spnav)
        endif(WIN32)

# -------------------------------- Shiboken/PySide ------------------------

        find_package(Shiboken QUIET)
        IF(SHIBOKEN_INCLUDE_DIR)
            message("-- Shiboken has been found.")
        ENDIF(SHIBOKEN_INCLUDE_DIR)
        find_package(PySide QUIET)
        IF(PYSIDE_INCLUDE_DIR)
            message("-- PySide has been found.")
        ENDIF(PYSIDE_INCLUDE_DIR)

# ------------------------------ Matplotlib ------------------------------

        find_package(Matplotlib)
        IF(MATPLOTLIB_FOUND)
            message("-- matplotlib-${MATPLOTLIB_VERSION} has been found.")
        ELSE(MATPLOTLIB_FOUND)
            message("matplotlib not found, Plot module will not available until matplotlib is installed!")
        ENDIF(MATPLOTLIB_FOUND)

# ------------------------------------------------------------------------


    endif(FREECAD_BUILD_GUI)


endif(FREECAD_LIBPACK_USE)

# copy build convenient files for M$
if(WIN32)
    if (EXISTS BuildAll.bat)
        configure_file(BuildAll.bat ${CMAKE_BINARY_DIR}/BuildAll.bat COPYONLY)
    endif (EXISTS BuildAll.bat)
    #configure_file(BuildAllNice.bat ${CMAKE_BINARY_DIR}/BuildAllNice.bat COPYONLY)
endif(WIN32)


include(cMake/FreeCadMacros.cmake)


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
	#SET (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GF /GY")
	SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DFC_DEBUG")
	# set default libs
	SET (CMAKE_C_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib SHFolder.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib ")
	set (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}")
	# set linker flag /nodefaultlib
	set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB")
	SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")

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
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthreads")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mthreads -Wl,--export-all-symbols")
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -mthreads -Wl,--export-all-symbols")
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
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "python, oce | opencascade, libqtgui4, libcoin60, libsoqt4-20, libxerces-c3.1, zlib1g, libboost-dev, libeigen2-dev")
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

