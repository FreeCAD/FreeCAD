set (
    Test_SRCS
    __init__.py
    Init.py
    BaseTests.py
    Document.py
    GuiDocument.py
    Metadata.py
    StringHasher.py
    Menu.py
    TestApp.py
    TestGui.py
    UnicodeTests.py
    UnitTests.py
    Workbench.py
    unittestgui.py
    testmakeWireString.py
    TestPythonSyntax.py
    TestPerf.py)

set (TestData_SRCS TestData/basic_metadata.xml TestData/bad_root_node.xml TestData/bad_xml.xml
                   TestData/bad_version.xml TestData/content_items.xml)

source_group ("" FILES ${Test_SRCS} ${TestData_SRCS})

if (BUILD_GUI)
    add_subdirectory (Gui)
    list (APPEND Test_SRCS InitGui.py)
endif (BUILD_GUI)

add_custom_target (Test ALL SOURCES ${Test_SRCS} ${TestData_SRCS})

fc_copy_sources (Test "${CMAKE_BINARY_DIR}/Mod/Test" ${Test_SRCS} ${TestData_SRCS})

install (FILES ${Test_SRCS} DESTINATION Mod/Test)

install (FILES ${TestData_SRCS} DESTINATION Mod/Test/TestData)
