
SET(Test_SRCS
    __init__.py
    Init.py
    BaseTests.py
    Document.py
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
)

SET(TestData_SRCS
    TestData/basic_metadata.xml
    TestData/bad_root_node.xml
    TestData/bad_xml.xml
    TestData/bad_version.xml
    TestData/content_items.xml
)

SOURCE_GROUP("" FILES ${Test_SRCS} ${TestData_SRCS})

if(BUILD_GUI)
    add_subdirectory(Gui)
    list (APPEND Test_SRCS InitGui.py)
endif(BUILD_GUI)

ADD_CUSTOM_TARGET(Test ALL
    SOURCES ${Test_SRCS} ${TestData_SRCS}
)

fc_copy_sources(Test "${CMAKE_BINARY_DIR}/Mod/Test" ${Test_SRCS} ${TestData_SRCS})

INSTALL(
    FILES
        ${Test_SRCS}
    DESTINATION
        Mod/Test
)

INSTALL(
    FILES
        ${TestData_SRCS}
    DESTINATION
        Mod/Test/TestData
)
