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
        ${CMAKE_CURRENT_BINARY_DIR}
)

set(StartGui_LIBS
        Start
        FreeCADGui
        )

set(StartGui_UIC_SRCS
        DlgStartPreferences.ui
    )

set(Start_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Start_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
        ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Start_TR_QRC} ${QM_SRCS})
qt_add_resources(Start_QRC_SRCS Resources/Start.qrc ${Start_TR_QRC})
# qtquick_compiler_add_resources(Start_QRC_SRCS Resources/Start.qrc ${Start_TR_QRC} qml.qrc)

SET(StartGui_SRCS
        ${Start_QRC_SRCS}
        ${StartGui_UIC_SRCS}
        DlgStartPreferencesImp.cpp
        DlgStartPreferencesImp.h
        AppStartGui.cpp
        FileCardDelegate.cpp
        FileCardDelegate.h
        FileCardView.cpp
        FileCardView.h
        FirstStartWidget.cpp
        FirstStartWidget.h
        FlowLayout.cpp
        FlowLayout.h
        GeneralSettingsWidget.cpp
        GeneralSettingsWidget.h
        Manipulator.cpp
        Manipulator.h
        NewFileButton.cpp
        NewFileButton.h
        PreCompiled.cpp
        PreCompiled.h
        StartView.cpp
        StartView.h
        ThemeSelectorWidget.cpp
        ThemeSelectorWidget.h
        )

SET(StartGuiIcon_SVG
        Resources/icons/StartCommandIcon.svg
        Resources/icons/PartDesignWorkbench.svg
        )

SET(StartGuiThumbnail_PNG
        Resources/thumbnails/Theme_thumbnail_classic.png
        Resources/thumbnails/Theme_thumbnail_dark.png
        Resources/thumbnails/Theme_thumbnail_light.png
        )

# TODO: Evaluate PCH use with Qt6/QtQuick/Qml
if (FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${StartGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(StartGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

# Add CoreFoundation to StartGui_LIBS on macOS
if (APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    if (COREFOUNDATION_LIBRARY)
        list(APPEND StartGui_LIBS ${COREFOUNDATION_LIBRARY})
    else()
        message(SEND_ERROR "CoreFoundation not found. Please ensure you're building on macOS.")
    endif()
endif()

add_library(StartGui SHARED ${StartGui_SRCS} ${StartGuiIcon_SVG} ${StartGuiThumbnail_PNG})
# target_link_libraries(StartGui ${StartGui_LIBS} Qt::Quick Qt::Qml Qt::QuickWidgets)
target_link_libraries(StartGui ${StartGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(StartGui)
endif()

SET_BIN_DIR(StartGui StartGui /Mod/Start)
SET_PYTHON_PREFIX_SUFFIX(StartGui)

fc_copy_sources(StartGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Start" ${StartGuiIcon_SVG})

INSTALL(TARGETS StartGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${StartGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Start/Resources/icons")
