# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

add_subdirectory(App)

target_link_libraries(Spreadsheet_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Spreadsheet
)
