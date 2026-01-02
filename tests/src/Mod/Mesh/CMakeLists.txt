# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)

target_link_libraries(Mesh_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Mesh
)
