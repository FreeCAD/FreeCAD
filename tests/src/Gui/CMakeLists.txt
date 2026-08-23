# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(Dialogs)
add_subdirectory(propertyeditor)

# Standard C++ GTest tests
add_executable(Gui_tests_run
        Assistant.cpp
        AsyncTaskRecompute.cpp
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

# Async preview tests use a QApplication. They are kept separate from
# Gui_tests_run, whose existing tests intentionally use QCoreApplication.
add_executable(AsyncPreviewTasks_Tests_run AsyncPreviewTasks.cpp)
add_test(NAME AsyncPreviewTasks_Tests_run COMMAND AsyncPreviewTasks_Tests_run)

target_include_directories(AsyncPreviewTasks_Tests_run PUBLIC SYSTEM ${QtTest_INCLUDE_DIRS})
target_link_libraries(AsyncPreviewTasks_Tests_run
    FreeCADApp
    FreeCADGui
    Part
    PartGui
    PartDesign
    ${QtTest_LIBRARIES}
)

if(WIN32)
    set_target_properties(AsyncPreviewTasks_Tests_run PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    set_tests_properties(AsyncPreviewTasks_Tests_run PROPERTIES
        LABELS "Qt"
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
else()
    set_target_properties(AsyncPreviewTasks_Tests_run PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    set_tests_properties(AsyncPreviewTasks_Tests_run PROPERTIES
        LABELS "Qt"
    )
endif()

# Qt tests
setup_qt_test(ObjectSelection)
setup_qt_test(QuantitySpinBox)

target_link_libraries(Gui_tests_run PRIVATE
    GTest::gmock_main
    FreeCADApp
    FreeCADGui
)
