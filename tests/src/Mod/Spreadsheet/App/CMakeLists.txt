# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

add_executable(Spreadsheet_tests_run
            PropertySheet.cpp
            RenameProperty.cpp
)

target_include_directories(Spreadsheet_tests_run PUBLIC
            ${CMAKE_BINARY_DIR}
)
