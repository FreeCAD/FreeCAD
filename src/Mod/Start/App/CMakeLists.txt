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

set(Start_LIBS
        FreeCADApp
        )

set(Start_SRCS
        AppStart.cpp
        DisplayedFilesModel.cpp
        DisplayedFilesModel.h
        ExamplesModel.cpp
        ExamplesModel.h
        CustomFolderModel.cpp
        CustomFolderModel.h
        FileUtilities.cpp
        FileUtilities.h
        PreCompiled.cpp
        PreCompiled.h
        RecentFilesModel.cpp
        RecentFilesModel.h
        ThumbnailSource.cpp
        ThumbnailSource.h)

add_library(Start SHARED ${Start_SRCS})
target_link_libraries(Start ${Start_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Start)
endif()

set_bin_dir(Start Start /Mod/Start)
set_python_prefix_suffix(Start)

install(TARGETS Start DESTINATION ${CMAKE_INSTALL_LIBDIR})
