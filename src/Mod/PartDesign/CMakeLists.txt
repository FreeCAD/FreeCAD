
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(PartDesign_Scripts
    __init__.py
    Init.py
    TestPartDesignApp.py
)

if(BUILD_GUI)
    list (APPEND PartDesign_Scripts
          InitGui.py
          TestPartDesignGui.py
          InvoluteGearFeature.py
          InvoluteGearFeature.ui
    )
endif(BUILD_GUI)

set(PartDesign_OtherScripts
    Scripts/__init__.py
    Scripts/DistanceBolt.py
    Scripts/Epitrochoid.py
    Scripts/FilletArc.py
    Scripts/Gear.py
    Scripts/Parallelepiped.py
    Scripts/RadialCopy.py
    Scripts/Spring.py
)

set(PartDesign_TestScripts
    PartDesignTests/__init__.py
    PartDesignTests/TestDatum.py
    PartDesignTests/TestShapeBinder.py
    PartDesignTests/TestPad.py
    PartDesignTests/TestPocket.py
    PartDesignTests/TestHole.py
    PartDesignTests/TestRevolve.py
    PartDesignTests/TestLoft.py
    PartDesignTests/TestPipe.py
    PartDesignTests/TestPrimitive.py
    PartDesignTests/TestMirrored.py
    PartDesignTests/TestLinearPattern.py
    PartDesignTests/TestPolarPattern.py
    PartDesignTests/TestMultiTransform.py
    PartDesignTests/TestBoolean.py
    PartDesignTests/TestFillet.py
    PartDesignTests/TestChamfer.py
    PartDesignTests/TestDraft.py
    PartDesignTests/TestThickness.py
)

set(PartDesign_GearScripts
    fcgear/__init__.py
    fcgear/fcgear.py
    fcgear/fcgeardialog.py
    fcgear/involute.py
    fcgear/svggear.py
)

set(PartDesign_WizardShaft
    WizardShaft/__init__.py
    WizardShaft/WizardShaft.svg
    WizardShaft/WizardShaft.py
    WizardShaft/WizardShaftTable.py
    WizardShaft/Shaft.py
    WizardShaft/ShaftFeature.py
    WizardShaft/ShaftDiagram.py
    WizardShaft/SegmentFunction.py
)

add_custom_target(PartDesignScripts ALL SOURCES
    ${PartDesign_Scripts}
    ${PartDesign_OtherScripts}
    ${PartDesign_TestScripts}
    ${PartDesign_GearScripts}
)

fc_target_copy_resource(PartDesignScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/PartDesign
    ${PartDesign_Scripts}
    ${PartDesign_OtherScripts}
    ${PartDesign_TestScripts}
    ${PartDesign_GearScripts}
)

INSTALL(
    FILES
        ${PartDesign_Scripts}
    DESTINATION
        Mod/PartDesign
)

INSTALL(
    FILES
        ${PartDesign_OtherScripts}
    DESTINATION
        Mod/PartDesign/Scripts
       )

INSTALL(
    FILES
        ${PartDesign_TestScripts}
    DESTINATION
        Mod/PartDesign/PartDesignTests
       )
        
INSTALL(
    FILES
        ${PartDesign_GearScripts}
    DESTINATION
        Mod/PartDesign/fcgear
        
)

if(BUILD_FEM)
    SET(WizardShaft_SRCS
        ${PartDesign_WizardShaft}
    )
    SOURCE_GROUP("wizardshaft" FILES ${WizardShaft_SRCS})

    SET(all_wizardshaft_files ${WizardShaft_SRCS})

    ADD_CUSTOM_TARGET(WizardShaft ALL
        SOURCES ${all_wizardshaft_files}
    )

    SET(all_files ${all_wizardshaft_files})

    fc_copy_sources(WizardShaft "${CMAKE_BINARY_DIR}/Mod/PartDesign" ${all_files})

    INSTALL(
        FILES
            ${WizardShaft_SRCS}
        DESTINATION
            Mod/PartDesign/WizardShaft
    )
endif(BUILD_FEM)

SET(FeatureHole_SRCS
    FeatureHole/__init__.py
    FeatureHole/HoleGui.py
    FeatureHole/FeatureHole.py
    FeatureHole/TaskHole.py
    FeatureHole/ViewProviderHole.py
    FeatureHole/Standards.py
    FeatureHole/PartDesign_Hole.svg
)
SOURCE_GROUP("featurehole" FILES ${FeatureHole_SRCS})

SET(FeatureHole_UI
    FeatureHole/TaskHole.ui
)

SET(all_featurehole_files ${FeatureHole_SRCS} ${FeatureHole_UI})
