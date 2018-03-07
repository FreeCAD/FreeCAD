
add_definitions(-DMOD_TECHDRAW_HANDLE_FACES=1)

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
        TestTechDrawApp.py
    DESTINATION
        Mod/TechDraw
)

INSTALL(
    DIRECTORY
        Templates
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    FILES_MATCHING
        PATTERN "*.svg*"
)

INSTALL(
    DIRECTORY
        PAT
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    FILES_MATCHING
        PATTERN "*.pat*"
)

INSTALL(
    DIRECTORY
        LineGroup
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    FILES_MATCHING
        PATTERN "*.csv*"
)

INSTALL(
    DIRECTORY
        Patterns
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    FILES_MATCHING
        PATTERN "*.svg*"
)


#unit test files
SET(TDTest_SRCS
    TDTest/__init__.py
    TDTest/DHatchTest.py
    TDTest/DProjGroupTest.py
    TDTest/DVAnnoSymImageTest.py
    TDTest/DVDimensionTest.py
    TDTest/DVPartTest.py
    TDTest/DVSectionTest.py
)

SET(TDTestFile_SRCS
    TDTest/TestHatch.svg
    TDTest/TestImage.png
    TDTest/TestSymbol.svg
    TDTest/TestTemplate.svg
)

SET(TDAllTest
    ${TDTest_SRCS}
    ${TDTestFile_SRCS}
)

ADD_CUSTOM_TARGET(TDTestTarget ALL
    SOURCES ${TDAllTest}
)

fc_copy_sources(TDTestTarget "${CMAKE_BINARY_DIR}/Mod/TechDraw" ${TDAllTest})

# install Python packages (for make install)
INSTALL(FILES ${TDTest_SRCS} DESTINATION Mod/TechDraw/TDTest)
INSTALL(FILES ${TDTestFile_SRCS} DESTINATION Mod/TechDraw/TDTest)

