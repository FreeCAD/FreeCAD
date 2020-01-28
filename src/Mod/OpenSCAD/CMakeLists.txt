IF (BUILD_GUI)
    PYSIDE_WRAP_RC(OpenSCAD_QRC_SRCS Resources/OpenSCAD.qrc)
ENDIF (BUILD_GUI)

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

set(all_files ${OpenSCAD_SRCS})

SET(OpenSCADGuiIcon_SVG
    Resources/icons/OpenSCADWorkbench.svg
)

ADD_CUSTOM_TARGET(OpenSCAD ALL
    SOURCES ${all_files} ${OpenSCAD_QRC_SRCS} ${OpenSCADGuiIcon_SVG}
)

fc_copy_sources(OpenSCAD "${CMAKE_BINARY_DIR}/Mod/OpenSCAD" ${all_files})

fc_copy_sources(OpenSCAD "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/OpenSCAD" ${OpenSCADGuiIcon_SVG})

IF (BUILD_GUI)
    fc_target_copy_resource(OpenSCAD
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/OpenSCAD
        OpenSCAD_rc.py)
ENDIF (BUILD_GUI)

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
INSTALL(
    FILES
        ${OpenSCADGuiIcon_SVG}
    DESTINATION
       "${CMAKE_INSTALL_DATADIR}/Mod/OpenSCAD/Resources/icons"
)

