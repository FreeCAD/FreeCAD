# Find OCULUS
#
# This module defines OCULUS_FOUND OCULUS_INCLUDE_DIRS OCULUS_LIBRARIES
#
# Copyright (c) 2012 I-maginer
#
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU
# Lesser General Public License as published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with this program;
# if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA, or go to http://www.gnu.org/copyleft/lesser.txt
#

# On a new cmake run, we do not need to be verbose
if (OCULUS_INCLUDE_DIR AND OCULUS_LIBRARY)
    set (OCULUS_FIND_QUIETLY FALSE)
endif ()

# If OCULUS_ROOT was defined in the environment, use it.
if (NOT OCULUS_ROOT)
    if (NOT "$ENV{OCULUS_ROOT}" STREQUAL "")
        set (OCULUS_ROOT $ENV{OCULUS_ROOT})
    else ()
        set (OCULUS_ROOT $ENV{SCOL_DEPENDENCIES_PATH}/oculus/LibOVR)
    endif ()
endif ()

# concat all the search paths
if (OCULUS_ROOT)
    set (OCULUS_INCLUDE_SEARCH_DIRS ${OCULUS_INCLUDE_SEARCH_DIRS} ${OCULUS_ROOT}/include)
    set (OCULUS_LIBRARY_SEARCH_RELEASE_DIRS ${OCULUS_LIBRARY_SEARCH_DIRS}
                                            ${OCULUS_ROOT}/Lib/x64/VS2012)
    set (OCULUS_LIBRARY_SEARCH_DEBUG_DIRS ${OCULUS_LIBRARY_SEARCH_DIRS}
                                          ${OCULUS_ROOT}/Lib/x64/VS2012)
endif ()

# log message
if (NOT OCULUS_FIND_QUIETLY)
    message (STATUS "Checking for OCULUS library")
endif ()

# Search for header files
find_path (OCULUS_INCLUDE_DIR OVR.h PATHS ${OCULUS_INCLUDE_SEARCH_DIRS})

# Search for libraries files (release mode)
find_library (OCULUS_LIBRARY_RELEASE libovr64 PATHS ${OCULUS_LIBRARY_SEARCH_RELEASE_DIRS})

# Search for libraries files (debug mode)
find_library (OCULUS_LIBRARY_DEBUG libovr64d PATHS ${OCULUS_LIBRARY_SEARCH_DEBUG_DIRS})

# Configure libraries for debug/release
set (
    OCULUS_INCLUDE_DIRS
    ${OCULUS_INCLUDE_DIR}
    CACHE PATH "Directory containing OCULUS header files")
set (OCULUS_LIBRARY debug ${OCULUS_LIBRARY_DEBUG} optimized ${OCULUS_LIBRARY_RELEASE})
set (
    OCULUS_LIBRARIES
    ${OCULUS_LIBRARY}
    CACHE STRING "OCULUS libraries files")

# IF(OCULUS_INCLUDE_DIR AND OCULUS_LIBRARY)
set (OCULUS_FOUND TRUE)
# ENDIF()

# Hide those variables in GUI
set (
    OCULUS_INCLUDE_DIR
    ${OCULUS_INCLUDE_DIR}
    CACHE INTERNAL "")
set (
    OCULUS_LIBRARY_RELEASE
    ${OCULUS_LIBRARY_RELEASE}
    CACHE INTERNAL "")
set (
    OCULUS_LIBRARY_DEBUG
    ${OCULUS_LIBRARY_DEBUG}
    CACHE INTERNAL "")
set (
    OCULUS_LIBRARY
    ${OCULUS_LIBRARY}
    CACHE INTERNAL "")

# log find result
if (OCULUS_FOUND)
    if (NOT OCULUS_FIND_QUIETLY)
        message (STATUS "  libraries: ${OCULUS_LIBRARIES}")
        message (STATUS "  includes: ${OCULUS_INCLUDE_DIRS}")
    endif ()
else (OCULUS_FOUND)
    if (NOT OCULUS_LIBRARIES)
        message (STATUS, "OCULUS library or one of it dependencies could not be found.")
    endif ()
    if (NOT OCULUS_INCLUDE_DIRS)
        message (STATUS "OCULUS include files could not be found.")
    endif ()
endif (OCULUS_FOUND)
