
SET(Arch_SRCS
    Init.py
    InitGui.py
    Component.py
    Cell.py
    Wall.py
    importIFC.py
    ifcReader.py
    IFC2X3_TC1.exp
    Arch_rc.py
    Arch.py
    Building.py
    Floor.py
    Site.py
    Structure.py
    Commands.py
    SectionPlane.py
    importDAE.py
    importOBJ.py
    Window.py
    Axis.py
)
SOURCE_GROUP("" FILES ${Arch_SRCS})

SET(all_files ${Arch_SRCS})

ADD_CUSTOM_TARGET(Arch ALL
    SOURCES ${all_files}
)

fc_copy_sources("Mod/Arch" "Arch" ${all_files})

INSTALL(
    FILES ${Arch_SRCS}
    DESTINATION Mod/Arch
)
