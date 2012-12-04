
add_subdirectory(App)
if(FREECAD_BUILD_GUI)
    add_subdirectory(Gui)
endif(FREECAD_BUILD_GUI)

INSTALL(
    FILES
        __init__.py
        Init.py
        InitGui.py
        TestPartDesignApp.py
        TestPartDesignGui.py
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

fc_copy_sources(Mod/PartDesign "${CMAKE_BINARY_DIR}/Mod/PartDesign" ${all_files})

INSTALL(
    FILES
        ${WizardShaft_SRCS}
    DESTINATION
        Mod/PartDesign/WizardShaft
)
