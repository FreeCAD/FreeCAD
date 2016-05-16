# - Find MED file installation
#
# The following variable are set:
#   MEDFILE_INCLUDE_DIRS
#   MEDFILE_LIBRARIES
#   MEDFILE_C_LIBRARIES
#   MEDFILE_F_LIBRARIES
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

# ------

MESSAGE(STATUS "Check for medfile (libmed and libmedc) ...")

# ------

SET(MEDFILE_ROOT_DIR $ENV{MEDFILE_ROOT_DIR} CACHE PATH "Path to the MEDFile.")
IF(MEDFILE_ROOT_DIR)
  LIST(APPEND CMAKE_PREFIX_PATH "${MEDFILE_ROOT_DIR}")
ENDIF(MEDFILE_ROOT_DIR)

FIND_PATH(MEDFILE_INCLUDE_DIRS med.h)
#FIND_PROGRAM(MDUMP mdump)
FIND_LIBRARY(MEDFILE_C_LIBRARIES NAMES medC)
FIND_LIBRARY(MEDFILE_F_LIBRARIES NAMES med)
IF(MEDFILE_F_LIBRARIES)
  SET(MEDFILE_LIBRARIES ${MEDFILE_C_LIBRARIES} ${MEDFILE_F_LIBRARIES})
ELSE(MEDFILE_F_LIBRARIES)
    SET(MEDFILE_LIBRARIES ${MEDFILE_C_LIBRARIES})
ENDIF(MEDFILE_F_LIBRARIES)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MEDFile REQUIRED_VARS MEDFILE_INCLUDE_DIRS MEDFILE_LIBRARIES)
