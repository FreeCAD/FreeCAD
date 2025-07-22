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

if ( EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/GSL/include" )
    include_directories( ${CMAKE_SOURCE_DIR}/src/3rdParty/GSL/include )
else()
    find_package(Microsoft.GSL)
    if( Microsoft.GSL_FOUND )
        message( STATUS "Found Microsoft.GSL: version ${Microsoft.GSL_VERSION}" )
    else()
        message( SEND_ERROR "The C++ Guidelines Support Library (GSL) submodule is not available. Please run git submodule update --init" )
    endif()
endif()

add_subdirectory(App)

set(Start_Scripts
        Init.py
        )

if (BUILD_GUI)
    add_subdirectory(Gui)
    list(APPEND Start_Scripts InitGui.py StartMigrator.py)
endif (BUILD_GUI)

add_custom_target(StartScripts ALL
        SOURCES ${Start_Scripts}
        )

fc_target_copy_resource(StartScripts
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Start
        ${Start_Scripts})

install(
        FILES
        ${Start_Scripts}
        DESTINATION
        Mod/Start
)
