# Standard C++ GTest tests
add_executable(Gui_tests_run
        Assistant.cpp
        Camera.cpp
        StyleParameters/StyleParametersApplicationTest.cpp
        StyleParameters/ParserTest.cpp
        StyleParameters/ParameterManagerTest.cpp
        InputHintTest.cpp
)

# Qt tests
setup_qt_test(QuantitySpinBox)

target_link_libraries(Gui_tests_run PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    ${Google_Tests_LIBS}
    FreeCADApp
    FreeCADGui
)
