# SPDX-License-Identifier: LGPL-2.1-or-later

add_executable(Zipios_tests_run
        collectioncollection.cpp
        zipfile.cpp
)


target_link_libraries(Zipios_tests_run PRIVATE
    GTest::gtest_main
    ${Python3_LIBRARIES}
    FreeCADApp
    zipios
)
