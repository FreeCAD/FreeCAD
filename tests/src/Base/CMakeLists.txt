
add_executable(
    Tests_Tools_run
    tst_Tools.cpp
)

target_link_libraries(Tests_Tools_run gtest gtest_main ${Google_Tests_LIBS} FreeCADBase)
