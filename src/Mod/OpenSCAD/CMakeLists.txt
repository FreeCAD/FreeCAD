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
    TestOpenSCADApp.py
)
IF (BUILD_GUI)
	LIST(APPEND OpenSCAD_SRCS TestOpenSCADGui.py)
ENDIF (BUILD_GUI)
SOURCE_GROUP("" FILES ${OpenSCAD_SRCS})

SET(OpenSCADTests_SRCS
    OpenSCADTest/__init__.py
    OpenSCADTest/test_information.md
)

SET(OpenSCADTestsApp_SRCS
    OpenSCADTest/app/__init__.py
    OpenSCADTest/app/test_importCSG.py
)

SET(OpenSCADTestsGui_SRCS
    OpenSCADTest/gui/__init__.py
    OpenSCADTest/gui/test_dummy.py
)

SET(OpenSCADTestsFiles_SRCS
    OpenSCADTest/data/__init__.py
    OpenSCADTest/data/CSG.scad
    OpenSCADTest/data/CSG.csg
    OpenSCADTest/data/Cube.stl
    OpenSCADTest/data/Square.dxf
    OpenSCADTest/data/Surface.dat
    OpenSCADTest/data/Surface2.dat
    OpenSCADTest/data/Surface.png
)

SET(OpenSCADTests_ALL
	${OpenSCADTests_SRCS}
	${OpenSCADTestsApp_SRCS}
	${OpenSCADTestsFiles_SRCS}
	)

IF (BUILD_GUI)
	LIST(APPEND OpenSCADTests_ALL ${OpenSCADTestsGui_SRCS})
ENDIF (BUILD_GUI)


set(all_files ${OpenSCAD_SRCS})

SET(OpenSCADGuiIcon_SVG
    Resources/icons/OpenSCADWorkbench.svg
)

ADD_CUSTOM_TARGET(OpenSCAD ALL
    SOURCES ${all_files} ${OpenSCAD_QRC_SRCS} ${OpenSCADGuiIcon_SVG}
)

ADD_CUSTOM_TARGET(OpenSCADTests ALL
    SOURCES ${OpenSCADTests_ALL}
)

fc_copy_sources(OpenSCAD "${CMAKE_BINARY_DIR}/Mod/OpenSCAD" ${all_files})

fc_copy_sources(OpenSCAD "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/OpenSCAD" ${OpenSCADGuiIcon_SVG})

fc_copy_sources(OpenSCADTests "${CMAKE_BINARY_DIR}/Mod/OpenSCAD" ${OpenSCADTests_ALL})

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

INSTALL(FILES ${OpenSCADTests_SRCS} DESTINATION Mod/OpenSCAD/OpenSCADTest)
INSTALL(FILES ${OpenSCADTestsApp_SRCS} DESTINATION Mod/OpenSCAD/OpenSCADTest/app)
INSTALL(FILES ${OpenSCADTestsGui_SRCS} DESTINATION Mod/OpenSCAD/OpenSCADTest/gui)
INSTALL(FILES ${OpenSCADTestsFiles_SRCS} DESTINATION Mod/OpenSCAD/OpenSCADTest/data)
