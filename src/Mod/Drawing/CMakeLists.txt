
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Drawing_Scripts
    README.md
    Init.py
    DrawingExample.py
    DrawingTests.py
    DrawingPatterns.py
)

if(BUILD_GUI)
    list (APPEND Drawing_Scripts InitGui.py)
endif(BUILD_GUI)

SET(Drawing_Templates
    Templates/A0_Landscape_ISO7200.svg
    Templates/A0_Landscape_plain.svg
    Templates/A0_Portrait_plain.svg
    Templates/A1_Landscape_ISO7200.svg
    Templates/A1_Landscape_plain.svg
    Templates/A1_Portrait_plain.svg
    Templates/A2_Landscape_ISO7200.svg
    Templates/A2_Landscape_plain.svg
    Templates/A2_Portrait_plain.svg
    Templates/A3_Landscape.svg
    Templates/A3_Landscape_ISO7200.svg
    Templates/A3_Landscape_plain.svg
    Templates/A3_Portrait_plain.svg
    Templates/A4_Landscape.svg
    Templates/A4_Landscape_ISO7200.svg
    Templates/A4_Landscape_plain.svg
    Templates/A4_Portrait_ISO7200.svg
    Templates/A4_Portrait_plain.svg
)

add_custom_target(DrawingScripts ALL
    SOURCES ${Drawing_Scripts} ${Drawing_Templates}
)

fc_target_copy_resource(DrawingScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Drawing
    ${Drawing_Scripts})

fc_target_copy_resource(DrawingScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Drawing
    ${Drawing_Templates})

INSTALL(
    FILES
        ${Drawing_Scripts}
    DESTINATION
        Mod/Drawing
)

INSTALL(
    DIRECTORY
        Templates
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Drawing
    FILES_MATCHING
        PATTERN "*.svg*"
        PATTERN "*.dxf*"
)
