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
        ${PYTHON_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIR}
        ${XercesC_INCLUDE_DIRS}
        ${QtCore_INCLUDE_DIRS}
        ${QtWidgets_INCLUDE_DIRS}
        ${QtSvg_INCLUDE_DIRS}
        #${QtNetwork_INCLUDE_DIRS}
        ${QtUiTools_INCLUDE_DIRS}
        #${QtQuick_INCLUDE_DIRS}
        #${QtQuickWidgets_INCLUDE_DIRS}
)

set(CleanStartGui_LIBS
        CleanStart
        FreeCADGui
        )

set(CleanStart_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/CleanStart_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
        ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${CleanStart_TR_QRC} ${QM_SRCS})
qt_add_resources(CleanStart_QRC_SRCS Resources/CleanStart.qrc ${CleanStart_TR_QRC})
# qtquick_compiler_add_resources(CleanStart_QRC_SRCS Resources/CleanStart.qrc ${CleanStart_TR_QRC} qml.qrc)

SET(CleanStartGui_SRCS
        ${CleanStart_QRC_SRCS}
        ${CleanStartGui_UIC_SRCS}
        AppCleanStartGui.cpp
        Command.cpp
        PreCompiled.cpp
        PreCompiled.h
        CleanStartView.cpp
        CleanStartView.h
        FileCardDelegate.cpp
        FileCardDelegate.h
        FileCardView.cpp
        FileCardView.h
        Workbench.cpp
        Workbench.h
        )

SET(CleanStartGuiIcon_SVG
        Resources/icons/CleanStartWorkbench.svg
        )

# TODO: Evaluate PCH use with Qt6/QtQuick/Qml
if (FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${CleanStartGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(CleanStartGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

add_library(CleanStartGui SHARED ${CleanStartGui_SRCS} ${CleanStartGuiIcon_SVG})
# target_link_libraries(CleanStartGui ${CleanStartGui_LIBS} Qt::Quick Qt::Qml Qt::QuickWidgets)
target_link_libraries(CleanStartGui ${CleanStartGui_LIBS})

SET_BIN_DIR(CleanStartGui CleanStartGui /Mod/CleanStart)
SET_PYTHON_PREFIX_SUFFIX(CleanStartGui)

fc_copy_sources(CleanStartGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/CleanStart" ${CleanStartGuiIcon_SVG})

INSTALL(TARGETS CleanStartGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${CleanStartGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/CleanStart/Resources/icons")
