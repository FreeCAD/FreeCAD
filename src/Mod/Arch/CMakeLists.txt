
SET(Arch_SRCS
    Init.py
    InitGui.py
    ArchComponent.py
    ArchWall.py
    importIFC.py
    ifcReader.py
    Arch_rc.py
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
    ifcWriter.py
)
SOURCE_GROUP("" FILES ${Arch_SRCS})

SET(all_files ${Arch_SRCS})

ADD_CUSTOM_TARGET(Arch ALL
    SOURCES ${all_files}
)

fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${all_files})

INSTALL(
    FILES ${Arch_SRCS}
    DESTINATION Mod/Arch
)
