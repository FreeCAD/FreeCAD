PYSIDE_WRAP_RC(Arch_QRC_SRCS Resources/Arch.qrc)

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
)
SOURCE_GROUP("" FILES ${Arch_SRCS})

ADD_CUSTOM_TARGET(Arch ALL
    SOURCES ${Arch_SRCS} ${Arch_QRC_SRCS}
)

fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Arch_SRCS})

fc_target_copy_resource(Arch
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Arch
    Arch_rc.py)

INSTALL(
    FILES
        ${Arch_SRCS}
        ${Arch_QRC_SRCS}
    DESTINATION Mod/Arch
)
