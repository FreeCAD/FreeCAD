# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif()

target_link_libraries(Part_tests_run
    GTest::gtest_main
    ${Python3_LIBRARIES}
    Part
)
