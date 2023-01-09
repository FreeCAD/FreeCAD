
add_executable(
    Tests_Tools_run
    tst_Tools.cpp
)

set (Tests_Tools_LIBS
    gtest
    gtest_main
    ${Google_Tests_LIBS}
    FreeCADBase
)

if(NOT BUILD_DYNAMIC_LINK_PYTHON)
    list(APPEND Tests_Tools_LIBS
        ${PYTHON_LIBRARIES}
    )
endif()

target_link_libraries(Tests_Tools_run ${Tests_Tools_LIBS})
