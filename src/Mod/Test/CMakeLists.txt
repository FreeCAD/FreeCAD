
SET(Test_SRCS
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
    InitGui.py
)
SOURCE_GROUP("" FILES ${Test_SRCS})

ADD_CUSTOM_TARGET(Test ALL
    SOURCES ${Test_SRCS}
)

fc_copy_sources("Mod/Test" "Test" ${Test_SRCS})

install(
    FILES
        ${Test_SRCS}
    DESTINATION
        Mod/Test
)

if(FREECAD_BUILD_GUI)
    add_subdirectory(Gui)
endif(FREECAD_BUILD_GUI)
