# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

add_subdirectory(App)

target_include_directories(Start_tests_run PUBLIC
    ${Python3_INCLUDE_DIRS}
)

target_link_libraries(Start_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Start
)
