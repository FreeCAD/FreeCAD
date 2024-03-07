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

add_subdirectory(App)

set(CleanStart_Scripts
        Init.py
        )

if (BUILD_GUI)
    add_subdirectory(Gui)
    list(APPEND CleanStart_Scripts InitGui.py)
endif (BUILD_GUI)

add_custom_target(CleanStartScripts ALL
        SOURCES ${CleanStart_Scripts}
        )

fc_target_copy_resource(CleanStartScripts
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/Mod/CleanStart
        ${CleanStart_Scripts})

INSTALL(
        FILES
        ${CleanStart_Scripts}
        DESTINATION
        Mod/CleanStart
)
