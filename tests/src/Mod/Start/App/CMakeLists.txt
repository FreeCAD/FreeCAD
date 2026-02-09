# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

add_executable(Start_tests_run
        FileUtilities.cpp
        ThumbnailSource.cpp
)

target_include_directories(
    Start_tests_run
        PUBLIC
            ${CMAKE_BINARY_DIR}
)
