# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

add_executable(Measure_tests_run
        MeasureDistance.cpp
)

target_include_directories(Measure_tests_run PUBLIC
        ${CMAKE_BINARY_DIR}
)
