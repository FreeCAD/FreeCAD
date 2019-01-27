IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Arch_QRC_SRCS Resources/Arch.qrc)
ENDIF (BUILD_GUI)

SET(Arch_SRCS
    Init.py
    InitGui.py
    ArchComponent.py
    ArchIFC.py
    ArchWall.py
    importIFC.py
    importIFClegacy.py
    Arch.py
    ArchBuilding.py
    ArchFloor.py
    ArchSite.py
    ArchStructure.py
    ArchCommands.py
    ArchSectionPlane.py
    importDAE.py
    importOBJ.py
    ArchWindow.py
    ArchAxis.py
    ArchVRM.py
    ArchRoof.py
    ArchStairs.py
    importWebGL.py
    importJSON.py
    ArchSpace.py
    ArchRebar.py
    TestArch.py
    ArchFrame.py
    ArchPanel.py
    ArchEquipment.py
    ArchCutPlane.py
    ArchMaterial.py
    ArchSchedule.py
    ArchProfile.py
    import3DS.py
    ArchPrecast.py
    importSH3D.py
    ArchPipe.py
    ArchNesting.py
    ArchBuildingPart.py
    ArchReference.py
)

SET(Dice3DS_SRCS
    Dice3DS/__init__.py
    Dice3DS/util.py
    Dice3DS/dom3ds.py
)

SET(Arch_presets
    Presets/profiles.csv
    Presets/pset_definitions.csv
)

SOURCE_GROUP("" FILES ${Arch_SRCS})

SET(ArchGuiIcon_SVG
    Resources/icons/ArchWorkbench.svg
)

ADD_CUSTOM_TARGET(Arch ALL
    SOURCES ${Arch_SRCS} ${Arch_QRC_SRCS} ${Dice3DS_SRCS} ${Arch_presets} ${ArchGuiIcon_SVG}
)

fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Arch_SRCS})
fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Dice3DS_SRCS})
fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Arch" ${ArchGuiIcon_SVG})

fc_target_copy_resource(Arch
    ${CMAKE_SOURCE_DIR}/src/Mod/Arch
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Arch
    ${Arch_presets}
)

IF (BUILD_GUI)
    fc_target_copy_resource(Arch
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Arch
        Arch_rc.py)
ENDIF (BUILD_GUI)

INSTALL(
    FILES
        ${Arch_SRCS}
        ${Arch_QRC_SRCS}
    DESTINATION Mod/Arch
)

INSTALL(
    FILES
        ${Dice3DS_SRCS}
    DESTINATION Mod/Arch/Dice3DS
)
INSTALL(
    DIRECTORY
        Presets
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Arch
    FILES_MATCHING PATTERN "*.csv*"
)
INSTALL(
    FILES
        ${ArchGuiIcon_SVG}
    DESTINATION
       "${CMAKE_INSTALL_DATADIR}/Mod/Arch/Resources/icons"
)

