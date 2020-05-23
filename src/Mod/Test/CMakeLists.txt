
SET(Test_SRCS
    __init__.py
    Init.py
    BaseTests.py
    Document.py
    Menu.py
    TestApp.py
    TestGui.py
    UnicodeTests.py
    UnitTests.py
    Workbench.py
    unittestgui.py
    testmakeWireString.py
    TestPythonSyntax.py
    AppTestSupport.py
    GuiTestSupport.py
)
SOURCE_GROUP("" FILES ${Test_SRCS})

if(BUILD_GUI)
    add_subdirectory(Gui)
    list (APPEND Test_SRCS InitGui.py)
endif(BUILD_GUI)

ADD_CUSTOM_TARGET(Test ALL
    SOURCES ${Test_SRCS}
)

fc_copy_sources(Test "${CMAKE_BINARY_DIR}/Mod/Test" ${Test_SRCS})

INSTALL(
    FILES
        ${Test_SRCS}
    DESTINATION
        Mod/Test
)
