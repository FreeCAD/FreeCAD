# SPDX-License-Identifier: LGPL-2.1-or-later

disable_occt8_deprecation_warnings()

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Sketcher_Scripts
    Init.py
    SketcherExample.py
    TestSketcherApp.py
    Profiles.py
    SketcherBlock.py
)

set(Sketcher_TestScripts
    SketcherTests/__init__.py
    SketcherTests/TestSketchFillet.py
    SketcherTests/TestSketcherSolver.py
    SketcherTests/TestSketchExpression.py
    SketcherTests/TestSketchValidateCoincidents.py
    SketcherTests/TestSketchCarbonCopyReverseMapping.py
    SketcherTests/TestSketchCarbonCopyReverseMapping.FCStd
    SketcherTests/TestSketchInternalFaces.py
    SketcherTests/TestSketcherEllipse.py
    SketcherTests/TestSketchBlocks.py
)

if(BUILD_GUI)
    list (APPEND Sketcher_Scripts
          InitGui.py
          TestSketcherGui.py
    )
    list (APPEND Sketcher_TestScripts
          SketcherTests/TestConstraintPreselectionGui.py
          SketcherTests/TestDistanceLabelExtensionGui.py
          SketcherTests/TestConstraintCommandsGui.py
          SketcherTests/TestCoincidentCommandGui.py
          SketcherTests/GuiTestCase.py
          SketcherTests/TestSketchBlocksGui.py
          SketcherTests/TestPlacementUpdate.py
          SketcherTests/TestOnViewParameterGui.py
          SketcherTests/TestExternalFacePreselection.py
    )
endif(BUILD_GUI)

set(Sketcher_Profiles
    ProfileLib/Hexagon.py
    ProfileLib/RegularPolygon.py
    ProfileLib/__init__.py
)

set(Sketcher_Blocks
    "Blocks/README.md"
    "Blocks/CE.txt"
    "Blocks/FCC.txt"
    "Blocks/Open hardware (no text).txt"
    "Blocks/Open hardware.txt"
    "Blocks/Recycling.txt"
    "Blocks/Sign - Attention.txt"
    "Blocks/Sign - High voltage.txt"
    "Blocks/Tidyman.txt"
    "Blocks/WEEE.txt"
)

add_custom_target(SketcherBlocks ALL SOURCES ${Sketcher_Blocks})
fc_target_copy_resource(SketcherBlocks
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Sketcher
    ${Sketcher_Blocks}
)
install(FILES ${Sketcher_Blocks} DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Sketcher/Blocks)

add_custom_target(SketcherScripts ALL
    SOURCES ${Sketcher_Scripts} ${Sketcher_Profiles} ${Sketcher_TestScripts}
)

fc_target_copy_resource(SketcherScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Sketcher
    ${Sketcher_Scripts}
    ${Sketcher_Profiles}
    ${Sketcher_TestScripts}
)

INSTALL(
    FILES
        ${Sketcher_Scripts}
    DESTINATION
        Mod/Sketcher
)

INSTALL(
    FILES
        ${Sketcher_Profiles}
    DESTINATION
        Mod/Sketcher/ProfileLib
)

INSTALL(
    FILES
        ${Sketcher_TestScripts}
    DESTINATION
        Mod/Sketcher/SketcherTests
       )
