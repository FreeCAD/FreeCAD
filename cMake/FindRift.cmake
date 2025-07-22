# Find OCULUS
#
# This module defines
#  OCULUS_FOUND
#  OCULUS_INCLUDE_DIRS
#  OCULUS_LIBRARIES
#
# Copyright (c) 2012 I-maginer
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA 02111-1307, USA, or go to
# http://www.gnu.org/copyleft/lesser.txt
#

# On a new cmake run, we do not need to be verbose
IF(OCULUS_INCLUDE_DIR AND OCULUS_LIBRARY)
	SET(OCULUS_FIND_QUIETLY FALSE)
ENDIF()

# If OCULUS_ROOT was defined in the environment, use it.
if (NOT OCULUS_ROOT)
  if(NOT "$ENV{OCULUS_ROOT}" STREQUAL "")
    set(OCULUS_ROOT $ENV{OCULUS_ROOT})
  else()
    set(OCULUS_ROOT $ENV{SCOL_DEPENDENCIES_PATH}/oculus/LibOVR)
  endif()
endif()

# concat all the search paths
IF(OCULUS_ROOT)
	SET(OCULUS_INCLUDE_SEARCH_DIRS
	  ${OCULUS_INCLUDE_SEARCH_DIRS}
	  ${OCULUS_ROOT}/include
  )
  SET(OCULUS_LIBRARY_SEARCH_RELEASE_DIRS
    ${OCULUS_LIBRARY_SEARCH_DIRS}
    ${OCULUS_ROOT}/Lib/x64/VS2012
  )
  SET(OCULUS_LIBRARY_SEARCH_DEBUG_DIRS
    ${OCULUS_LIBRARY_SEARCH_DIRS}
    ${OCULUS_ROOT}/Lib/x64/VS2012
  )
ENDIF()

# log message
IF (NOT OCULUS_FIND_QUIETLY)
	MESSAGE(STATUS "Checking for OCULUS library")
ENDIF()

# Search for header files
FIND_PATH(OCULUS_INCLUDE_DIR OVR.h
  PATHS ${OCULUS_INCLUDE_SEARCH_DIRS})

# Search for libraries files (release mode)
FIND_LIBRARY(OCULUS_LIBRARY_RELEASE libovr64
  PATHS ${OCULUS_LIBRARY_SEARCH_RELEASE_DIRS})

# Search for libraries files (debug mode)
FIND_LIBRARY(OCULUS_LIBRARY_DEBUG libovr64d
  PATHS ${OCULUS_LIBRARY_SEARCH_DEBUG_DIRS})

# Configure libraries for debug/release
SET(OCULUS_INCLUDE_DIRS ${OCULUS_INCLUDE_DIR} CACHE PATH "Directory containing OCULUS header files")
SET(OCULUS_LIBRARY debug ${OCULUS_LIBRARY_DEBUG} optimized ${OCULUS_LIBRARY_RELEASE})
SET(OCULUS_LIBRARIES ${OCULUS_LIBRARY} CACHE STRING "OCULUS libraries files")

#IF(OCULUS_INCLUDE_DIR AND OCULUS_LIBRARY)
	SET(OCULUS_FOUND TRUE)
#ENDIF()

# Hide those variables in GUI
SET(OCULUS_INCLUDE_DIR ${OCULUS_INCLUDE_DIR} CACHE INTERNAL "")
SET(OCULUS_LIBRARY_RELEASE ${OCULUS_LIBRARY_RELEASE} CACHE INTERNAL "")
SET(OCULUS_LIBRARY_DEBUG ${OCULUS_LIBRARY_DEBUG} CACHE INTERNAL "")
SET(OCULUS_LIBRARY ${OCULUS_LIBRARY} CACHE INTERNAL "")

# log find result
IF(OCULUS_FOUND)
	IF(NOT OCULUS_FIND_QUIETLY)
		MESSAGE(STATUS "  libraries: ${OCULUS_LIBRARIES}")
		MESSAGE(STATUS "  includes: ${OCULUS_INCLUDE_DIRS}")
	ENDIF()
ELSE(OCULUS_FOUND)
	IF(NOT OCULUS_LIBRARIES)
		MESSAGE(STATUS, "OCULUS library or one of it dependencies could not be found.")
	ENDIF()
	IF(NOT OCULUS_INCLUDE_DIRS)
		MESSAGE(STATUS "OCULUS include files could not be found.")
	ENDIF()
ENDIF(OCULUS_FOUND)