PYSIDE_WRAP_RC(OpenSCAD_QRC_SRCS Resources/OpenSCAD.qrc)

SET(OpenSCAD_SRCS
        Init.py
        InitGui.py
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
    SOURCES ${allfiles} ${OpenSCAD_QRC_SRCS}
)

fc_copy_sources(OpenSCAD "${CMAKE_BINARY_DIR}/Mod/OpenSCAD" ${all_files})

fc_target_copy_resource(OpenSCAD
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/Mod/OpenSCAD
    OpenSCAD_rc.py)

INSTALL(
    FILES
        ${ply_SRCS}
    DESTINATION
        Mod/OpenSCAD/ply
)
INSTALL(
    FILES
        ${OpenSCAD_SRCS}
        ${OpenSCAD_QRC_SRCS}
    DESTINATION
        Mod/OpenSCAD
)
