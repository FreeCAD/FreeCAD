# - Find MED file installation
#
# The following variable are set:
#   MEDFILE_INCLUDE_DIRS
#   MEDFILE_LIBRARIES
#   MEDFILE_C_LIBRARIES
#   MEDFILE_F_LIBRARIES
#   MEDFILE_VERSION
#
#  The CMake (or environment) variable MEDFILE_ROOT_DIR can be set to
#  guide the detection and indicate a root directory to look into.
#
############################################################################
# Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

if(MEDFile_FIND_VERSION OR MEDFile_FIND_COMPONENTS)
    # Those aren't implemented (yet), but can be at a later date.
    message(FATAL_ERROR "FreeCAD FindMEDFile.cmake doesn't support version matching or components")
endif()

set(_prefer_system_default ON)
if(FREECAD_LIBPACK_USE)
    # Don't use MEDFile config bundled with the LibPack, it is broken.
    # See https://github.com/FreeCAD/FreeCAD-LibPack/issues/82
    set(_prefer_system_default OFF)
endif()
if(((NOT DEFINED FREECAD_CMAKE_PREFER_SYSTEM_MEDFILE) AND _prefer_system_default) OR
        FREECAD_CMAKE_PREFER_SYSTEM_MEDFILE)
    set(_medfile_find_mode "config")
    find_package(MEDFile NO_MODULE)
    if(NOT MEDFile_FOUND)
        set(_cmake_module_path "${CMAKE_MODULE_PATH}")
        list(REMOVE_ITEM CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cMake")
        set(_medfile_find_mode "module")
        find_package(MEDFile)
        set(CMAKE_MODULE_PATH "${_cmake_module_path}")
    endif()

    if(MEDFile_FOUND)
        if(NOT TARGET medC)
            message(FATAL_ERROR
                "Imported MEDFile (${_medfile_find_mode} mode) but no medC target exists")
        endif()

        # Target medC might not have an INCLUDE_DIRECTORIES allowing for #include <med.h>,
        # guard against this by finding the header ourselves and appending the include dir
        # if necessary.
        get_target_property(_medc_includes medC INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT _medc_includes)
            set(_medc_includes "")
        endif()
        find_file(_med_h med.h PATHS "${_medc_includes}" NO_PACKAGE_ROOT_PATH NO_CACHE)
        if(NOT _med_h)
            find_file(_med_h med/med.h PATHS "${_medc_includes}" PATH_SUFFIXES med NO_PACKAGE_ROOT_PATH NO_CACHE)
            if(NOT _med_h)
                message(FATAL_ERROR "Imported MEDFile (${_medfile_find_mode} mode) but neither med.h or med/med.h were found")
            endif()
            get_filename_component(_med_h_dir "${_med_h}" DIRECTORY)
            list(APPEND _medc_includes "${_med_h_dir}")
            message(STATUS "find_package(MEDFile)'s medC target was missing the ${_medc_includes} include dir, appendng")
            set_target_properties(medC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_medc_includes}")
        endif()

        # MEDFile depends on HDF5, it might have added _FORTIFY_SOURCE defines to the build.
        hdf5_clean_fortify_source()

        # CMake found it, we're done here.
        message(STATUS "Passed find_package(MEDFile) through (${_medfile_find_mode} mode)")
        return()
    endif()
endif()

# ------

MESSAGE(STATUS "Check for medfile (libmed and libmedc) ...")

# ------

SET(MEDFILE_ROOT_DIR $ENV{MEDFILE_ROOT_DIR} CACHE PATH "Path to the MEDFile.")
IF(MEDFILE_ROOT_DIR)
  LIST(APPEND CMAKE_PREFIX_PATH "${MEDFILE_ROOT_DIR}")
ENDIF(MEDFILE_ROOT_DIR)

FIND_PATH(MEDFILE_INCLUDE_DIRS med.h PATH_SUFFIXES med)
FIND_FILE(meddotH med.h PATHS ${MEDFILE_INCLUDE_DIRS} NO_DEFAULT_PATH)
IF(NOT meddotH)
	MESSAGE(FATAL_ERROR "med.h not found, please install development header-files for libmedc")
ENDIF(NOT meddotH)

function(medfile_extract_med_h_data)
    file(READ ${meddotH} _med_h)

    # Try and infer the required HDF5 version from med.h,
    string(FIND "${_med_h}" "#define MED_HAVE_MPI" matchres)
    if(${matchres} EQUAL -1)
        message(STATUS "We guess that libmed was built using hdf5-serial version")
        set(HDF5_VARIANT "hdf5-serial" PARENT_SCOPE)
    else()
        message(STATUS "We guess that libmed was built using hdf5-openmpi version")
        set(HDF5_VARIANT "hdf5-openmpi;hdf5_openmpi" PARENT_SCOPE)
        set(HDF5_PREFER_PARALLEL TRUE PARENT_SCOPE) # Used by find_package(HDF5)
    endif()

    # Extract version info
    string(REGEX MATCH "define[ \t]+MED_MAJOR_NUM[ \t]+([0-9?])" _med_major_version_match "${_med_h}")
    set(MED_MAJOR_VERSION "${CMAKE_MATCH_1}" PARENT_SCOPE)
    string(REGEX MATCH "define[ \t]+MED_MINOR_NUM[ \t]+([0-9?])" _med_minor_version_match "${_med_h}")
    set(MED_MINOR_VERSION "${CMAKE_MATCH_1}" PARENT_SCOPE)
    string(REGEX MATCH "define[ \t]+MED_RELEASE_NUM[ \t]+([0-9?])" _med_release_version_match "${_med_h}")
    set(MED_RELEASE_VERSION "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set(MEDFILE_VERSION "${MED_MAJOR_VERSION}.${MED_MINOR_VERSION}.${MED_RELEASE_VERSION}" PARENT_SCOPE)
endfunction()

medfile_extract_med_h_data()

#FIND_PROGRAM(MDUMP mdump)
find_library(MEDFILE_C_LIBRARIES NAMES medC)
find_library(MEDFILE_F_LIBRARIES NAMES med)

add_library(medC INTERFACE IMPORTED)
if(MEDFILE_F_LIBRARIES)
    set(MEDFILE_LIBRARIES "${MEDFILE_C_LIBRARIES};${MEDFILE_F_LIBRARIES}")
else()
    set(MEDFILE_LIBRARIES "${MEDFILE_C_LIBRARIES}")
endif()
target_include_directories(medC INTERFACE "${MEDFILE_INCLUDE_DIRS}")
target_link_libraries(medC INTERFACE "${MEDFILE_LIBRARIES}")

# Find the HDF5 dependency
if(MSVC)
    find_package(HDF5 REQUIRED COMPONENTS C HL static)
else()
    find_package(HDF5 REQUIRED)
endif()
target_link_libraries(medC INTERFACE hdf5::hdf5)

if(NOT MSVC)
    # Med Fichier can require MPI
    pkg_search_module(OPENMPI ompi-cxx)
    if(NOT OPENMPI_FOUND)
        message(WARNING "ompi-cxx was not found. Check for error above.")
    endif()
    target_compile_options(medC INTERFACE "${OPENMPI_CFLAGS}")
    target_include_directories(medC INTERFACE "${OPENMPI_LIBRARY_DIRS}")
    target_link_libraries(medC INTERFACE "${OPENMPI_LIBRARIES}")
endif()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MEDFile REQUIRED_VARS MEDFILE_INCLUDE_DIRS MEDFILE_LIBRARIES)
