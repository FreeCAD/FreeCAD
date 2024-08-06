# * Find MED file installation
#
# The following variable are set: MEDFILE_INCLUDE_DIRS MEDFILE_LIBRARIES MEDFILE_C_LIBRARIES
# MEDFILE_F_LIBRARIES MEDFILE_VERSION
#
# The CMake (or environment) variable MEDFILE_ROOT_DIR can be set to guide the detection and
# indicate a root directory to look into.
#
# ##################################################################################################
# Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
#
# This library is free software; you can redistribute it and/or modify it under the terms of the GNU
# Lesser General Public License as published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with this library;
# if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

# ------

message(STATUS "Check for medfile (libmed and libmedc) ...")

# ------

set(MEDFILE_ROOT_DIR
    $ENV{MEDFILE_ROOT_DIR}
    CACHE PATH "Path to the MEDFile.")
if(MEDFILE_ROOT_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${MEDFILE_ROOT_DIR}")
endif(MEDFILE_ROOT_DIR)

find_path(MEDFILE_INCLUDE_DIRS med.h PATH_SUFFIXES med)
find_file(
    meddotH med.h
    PATHS ${MEDFILE_INCLUDE_DIRS}
    NO_DEFAULT_PATH)
if(NOT meddotH)
    message(FATAL_ERROR "med.h not found, please install development header-files for libmedc")
endif(NOT meddotH)
# FIND_PROGRAM(MDUMP mdump)
find_library(MEDFILE_C_LIBRARIES NAMES medC)
find_library(MEDFILE_F_LIBRARIES NAMES med)
if(MEDFILE_F_LIBRARIES)
    set(MEDFILE_LIBRARIES ${MEDFILE_C_LIBRARIES} ${MEDFILE_F_LIBRARIES})
else(MEDFILE_F_LIBRARIES)
    set(MEDFILE_LIBRARIES ${MEDFILE_C_LIBRARIES})
endif(MEDFILE_F_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MEDFile REQUIRED_VARS MEDFILE_INCLUDE_DIRS MEDFILE_LIBRARIES)

if(meddotH)
    file(READ "${meddotH}" _med_h)
    string(REGEX MATCH "define[ \t]+MED_MAJOR_NUM[ \t]+([0-9?])" _med_major_version_match
                 "${_med_h}")
    set(MED_MAJOR_VERSION "${CMAKE_MATCH_1}")
    string(REGEX MATCH "define[ \t]+MED_MINOR_NUM[ \t]+([0-9?])" _med_minor_version_match
                 "${_med_h}")
    set(MED_MINOR_VERSION "${CMAKE_MATCH_1}")
    string(REGEX MATCH "define[ \t]+MED_RELEASE_NUM[ \t]+([0-9?])" _med_release_version_match
                 "${_med_h}")
    set(MED_RELEASE_VERSION "${CMAKE_MATCH_1}")
    set(MEDFILE_VERSION "${MED_MAJOR_VERSION}.${MED_MINOR_VERSION}.${MED_RELEASE_VERSION}")
endif()
