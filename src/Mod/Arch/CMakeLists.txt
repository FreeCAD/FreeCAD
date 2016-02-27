IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Arch_QRC_SRCS Resources/Arch.qrc)
ENDIF (BUILD_GUI)

SET(Arch_SRCS
    Init.py
    InitGui.py
    ArchComponent.py
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
    ArchSpace.py
    ArchRebar.py
    TestArch.py
    ArchFrame.py
    ArchPanel.py
    ArchEquipment.py
    ArchCutPlane.py
    ArchServer.py
    ArchMaterial.py
    ArchSchedule.py
    ArchProfile.py
)

SET(Arch_presets
    Presets/profiles.csv
)

SOURCE_GROUP("" FILES ${Arch_SRCS})

ADD_CUSTOM_TARGET(Arch ALL
    SOURCES ${Arch_SRCS} ${Arch_QRC_SRCS}
)

fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Arch_SRCS})

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
    DIRECTORY
        Presets
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Arch
    FILES_MATCHING PATTERN "*.csv*"
)

SET(ArchGuiIcon_SVG
    Resources/icons/ArchWorkbench.svg
)

fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Arch" ${ArchGuiIcon_SVG})

INSTALL(FILES ${ArchGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Arch/Resources/icons")
