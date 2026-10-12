# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)

if(BUILD_GUI)
    target_sources(Part_tests_run PRIVATE
        Gui/SoBrepEdgeSet.cpp
    )
    target_link_libraries(Part_tests_run PartGui)
endif()

target_link_libraries(Part_tests_run
    GTest::gtest_main
    ${Python3_LIBRARIES}
    Part
)
