# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(Dialogs)
add_subdirectory(propertyeditor)

# Standard C++ GTest tests
add_executable(Gui_tests_run
        Assistant.cpp
        Camera.cpp
        FileDialog.cpp
        StyleParameters/StyleParametersApplicationTest.cpp
        StyleParameters/ParserTest.cpp
        StyleParameters/ParameterManagerTest.cpp
        StyleParameters/YamlParameterSourceTest.cpp
        InputHintTest.cpp
        SelectionTest.cpp
)

# Qt tests
setup_qt_test(QuantitySpinBox SelectionMenu)
if(TARGET PartGui)
    setup_qt_test(SelectionRendering)
    target_link_libraries(SelectionRendering_Tests_run
        PartGui
        ${QtOpenGL_LIBRARIES}
    )
endif()

target_link_libraries(Gui_tests_run PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    FreeCADApp
    FreeCADGui
)
