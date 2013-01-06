SET(OpenSCAD_SRCS
        Init.py
        InitGui.py
        OpenSCAD_rc.py
        OpenSCAD2Dgeom.py
        OpenSCADFeatures.py
        OpenSCADUtils.py
        OpenSCADCommands.py
        exportCSG.py
        importCSG.py
        tokrules.py
        colorcodeshapes.py
        expandplacements.py
        replaceobj.py
)
SOURCE_GROUP("" FILES ${OpenSCAD_SRCS})

SET(ply_SRCS
        ply/lex.py
        ply/README
        ply/yacc.py
        ply/__init__.py
)
SOURCE_GROUP("ply" FILES ${ply_SRCS})

set(all_files ${OpenSCAD_SRCS} ${ply_SRCS})

ADD_CUSTOM_TARGET(OpenSCAD ALL
    SOURCES ${allfiles}
)

fc_copy_sources(OpenSCAD "${CMAKE_BINARY_DIR}/Mod/OpenSCAD" ${all_files})

INSTALL(
    FILES
        ${ply_SRCS}
    DESTINATION
        Mod/OpenSCAD/ply
)
INSTALL(
    FILES
        ${OpenSCAD_SRCS}
    DESTINATION
        Mod/OpenSCAD
)
