# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(Dialogs)
add_subdirectory(propertyeditor)

# Standard C++ GTest tests
add_executable(Gui_tests_run
        Assistant.cpp
        Camera.cpp
        ColorScaleOverlayTest.cpp
        FileDialog.cpp
        StyleParameters/DiagnosticsCapture.h
        StyleParameters/ValueMatchers.h
        StyleParameters/ValueMatchers.cpp
        StyleParameters/StyleParametersApplicationTest.cpp
        StyleParameters/ParserTest.cpp
        StyleParameters/ParserRobustnessTest.cpp
        StyleParameters/ParameterManagerTest.cpp
        StyleParameters/YamlParameterSourceTest.cpp
        StyleParameters/DiagnosticsTest.cpp
        StyleParameters/ValueAccessTest.cpp
        StyleParameters/EdgesTest.cpp
        StyleParameters/CornersTest.cpp
        StyleParameters/GradientTest.cpp
        StyleParameters/ColorShadingTest.cpp
        InputHintTest.cpp
        ViewProviderDoubleClick.cpp
        SelectionTest.cpp
)

# Qt tests
setup_qt_test(ObjectSelection)
setup_qt_test(TranslatorLocale)
setup_qt_test(InputField)
setup_qt_test(QuantitySpinBox SelectionMenu)
setup_qt_test(NumericLocaleConsumers)
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
