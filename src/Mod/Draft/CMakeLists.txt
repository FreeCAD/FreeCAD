PYSIDE_WRAP_RC(Draft_QRC_SRCS Resources/Draft.qrc)

SET(Draft_SRCS
    ${Draft_QRC_SRCS}
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
    importDXF.py
    importOCA.py
    importSVG.py
    importDWG.py
    importAirfoilDAT.py
    TestDraft.py
)
SOURCE_GROUP("" FILES ${Draft_SRCS})

SET(all_files ${Draft_SRCS})

ADD_CUSTOM_TARGET(Draft ALL
    SOURCES ${all_files}
)

fc_copy_sources(Draft "${CMAKE_BINARY_DIR}/Mod/Draft" ${all_files})

INSTALL(
    FILES
        ${Draft_SRCS}
    DESTINATION
        Mod/Draft
)
