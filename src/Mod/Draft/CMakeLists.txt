add_subdirectory(App)

IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Draft_QRC_SRCS Resources/Draft.qrc)
ENDIF (BUILD_GUI)

SET(Draft_SRCS
    Init.py
    InitGui.py
    Draft.py
    DraftTools.py
    DraftGui.py
    DraftSnap.py
    DraftTrackers.py
    DraftVecUtils.py
    DraftGeomUtils.py
    WorkingPlane.py
    getSVG.py
    importDXF.py
    importOCA.py
    importSVG.py
    importDWG.py
    importAirfoilDAT.py
    TestDraft.py
)
SOURCE_GROUP("" FILES ${Draft_SRCS})

SET(DraftGuiIcon_SVG
    Resources/icons/DraftWorkbench.svg
)

ADD_CUSTOM_TARGET(Draft ALL
    SOURCES ${Draft_SRCS} ${Draft_QRC_SRCS} ${DraftGuiIcon_SVG}
)

fc_copy_sources(Draft "${CMAKE_BINARY_DIR}/Mod/Draft" ${Draft_SRCS})

fc_copy_sources(Draft "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Draft" ${DraftGuiIcon_SVG})

IF (BUILD_GUI)
    fc_target_copy_resource(Draft
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Draft
        Draft_rc.py)
ENDIF (BUILD_GUI)

INSTALL(
    FILES
        ${Draft_SRCS}
        ${Draft_QRC_SRCS}
    DESTINATION
        Mod/Draft
)

INSTALL(
    FILES
        ${DraftGuiIcon_SVG}
    DESTINATION
       "${CMAKE_INSTALL_DATADIR}/Mod/Draft/Resources/icons"
)

