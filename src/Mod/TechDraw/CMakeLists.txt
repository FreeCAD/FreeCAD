
add_definitions(-DMOD_TECHDRAW_HANDLE_FACES=1)

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(TechDraw_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND TechDraw_Scripts InitGui.py)
endif(BUILD_GUI)

SET(TechDraw_PATFile
    PAT/FCPAT.pat
)

SET(TechDraw_LineGroupFile
    LineGroup/LineGroup.csv
)

add_custom_target(TechDraw_Data ALL
    SOURCES ${TechDraw_Scripts} ${TechDraw_PATFile} ${TechDraw_LineGroupFile}
)

fc_target_copy_resource(TechDraw_Data
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/TechDraw
    ${TechDraw_Scripts}
)

fc_target_copy_resource(TechDraw_Data
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    ${TechDraw_PATFile})

fc_target_copy_resource(TechDraw_Data
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    ${TechDraw_LineGroupFile})

INSTALL(
    FILES
        ${TechDraw_Scripts}
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
)

SET(TDTestFile_SRCS
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

