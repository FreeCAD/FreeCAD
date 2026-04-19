# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

setup_qt_test(AsyncPreviewControllerTest)
setup_qt_test(DeferredDialogRejectTest)

target_include_directories(AsyncPreviewControllerTest_Tests_run PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_include_directories(DeferredDialogRejectTest_Tests_run PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
