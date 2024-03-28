# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#  *                                                                          *
#  *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
#  *                                                                          *
#  *   This file is part of FreeCAD.                                          *
#  *                                                                          *
#  *   FreeCAD is free software: you can redistribute it and/or modify it     *
#  *   under the terms of the GNU Lesser General Public License as            *
#  *   published by the Free Software Foundation, either version 2.1 of the   *
#  *   License, or (at your option) any later version.                        *
#  *                                                                          *
#  *   FreeCAD is distributed in the hope that it will be useful, but         *
#  *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
#  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#  *   Lesser General Public License for more details.                        *
#  *                                                                          *
#  *   You should have received a copy of the GNU Lesser General Public       *
#  *   License along with FreeCAD. If not, see                                *
#  *   <https://www.gnu.org/licenses/>.                                       *
#  *                                                                          *
#  ***************************************************************************/

include_directories(
        ${PYTHON_INCLUDE_DIRS}
        ${QtCore_INCLUDE_DIRS}
)

set(CleanStart_LIBS
        FreeCADApp
        )

SET(CleanStart_SRCS
        AppCleanStart.cpp
        DisplayedFilesModel.cpp
        DisplayedFilesModel.h
        ExamplesModel.cpp
        ExamplesModel.h
        PreCompiled.cpp
        PreCompiled.h
        RecentFilesModel.cpp
        RecentFilesModel.h)

add_library(CleanStart SHARED ${CleanStart_SRCS})
target_link_libraries(CleanStart ${CleanStart_LIBS})

SET_BIN_DIR(CleanStart CleanStart /Mod/CleanStart)
SET_PYTHON_PREFIX_SUFFIX(CleanStart)

INSTALL(TARGETS CleanStart DESTINATION ${CMAKE_INSTALL_LIBDIR})
