# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(Dialogs)
add_subdirectory(propertyeditor)

# Standard C++ GTest tests
add_executable(Gui_tests_run
        Assistant.cpp
        Camera.cpp
        ColorScaleOverlayTest.cpp
        FileDialog.cpp
        StyleParameters/StyleParametersApplicationTest.cpp
        StyleParameters/ParserTest.cpp
        StyleParameters/ParameterManagerTest.cpp
        StyleParameters/YamlParameterSourceTest.cpp
        InputHintTest.cpp
        SelectionTest.cpp
)

# Qt tests
setup_qt_test(ObjectSelection)
setup_qt_test(QuantitySpinBox)
setup_qt_test(ViewerLifetime)
setup_qt_test(ViewerLifetimeShutdown)
setup_qt_test(EditableDatumLabelLifetimeShutdown)

add_executable(PyResourceLifetimeShutdown PyResourceLifetimeShutdown.cpp)
target_link_libraries(PyResourceLifetimeShutdown PRIVATE
    FreeCADApp
    FreeCADGui
    ${Python3_LIBRARIES}
)
add_test(NAME PyResourceLifetimeShutdown COMMAND PyResourceLifetimeShutdown)

add_executable(GeneratedWrapperLifetimeShutdown GeneratedWrapperLifetimeShutdown.cpp)
target_link_libraries(GeneratedWrapperLifetimeShutdown PRIVATE
    FreeCADApp
    FreeCADGui
    ${Python3_LIBRARIES}
)
add_test(NAME GeneratedWrapperLifetimeShutdown COMMAND GeneratedWrapperLifetimeShutdown)

target_link_libraries(Gui_tests_run PRIVATE
    GTest::gmock_main
    FreeCADApp
    FreeCADGui
)
