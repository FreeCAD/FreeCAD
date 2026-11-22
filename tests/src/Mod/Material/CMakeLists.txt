# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)

target_link_libraries(Material_tests_run
    GTest::gtest_main
    ${Python3_LIBRARIES}
    Materials
)
