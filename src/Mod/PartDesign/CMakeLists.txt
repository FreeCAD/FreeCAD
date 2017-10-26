
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        __init__.py
        Init.py
        InitGui.py
        TestPartDesignApp.py
        TestPartDesignGui.py
        InvoluteGearFeature.py
        InvoluteGearFeature.ui
    DESTINATION
        Mod/PartDesign
)

INSTALL(
    FILES
        Scripts/__init__.py
        Scripts/DistanceBolt.py
        Scripts/Epitrochoid.py
        Scripts/FilletArc.py
        Scripts/Gear.py
        Scripts/Parallelepiped.py
        Scripts/RadialCopy.py
        Scripts/Spring.py
    DESTINATION
        Mod/PartDesign/Scripts
       )

INSTALL(
    FILES
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
    DESTINATION
        Mod/PartDesign/PartDesignTests
       )
        
INSTALL(
    FILES
        fcgear/__init__.py
        fcgear/fcgear.py
        fcgear/fcgeardialog.py
        fcgear/involute.py
        fcgear/svggear.py
    DESTINATION
        Mod/PartDesign/fcgear
        
)

if(BUILD_FEM)
    SET(WizardShaft_SRCS
        WizardShaft/__init__.py
        WizardShaft/WizardShaft.svg
        WizardShaft/WizardShaft.py
        WizardShaft/WizardShaftTable.py
        WizardShaft/Shaft.py
        WizardShaft/ShaftFeature.py
        WizardShaft/ShaftDiagram.py
        WizardShaft/SegmentFunction.py
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

#ADD_CUSTOM_TARGET(FeatureHole ALL
#    SOURCES ${all_featurehole_files}
#)

#SET(all_files ${all_featurehole_files})

#fc_copy_sources(FeatureHole "${CMAKE_BINARY_DIR}/Mod/PartDesign" ${all_files})

#INSTALL(
#    FILES
#        ${FeatureHole_SRCS}
#    DESTINATION
#        Mod/PartDesign/FeatureHole
#)
