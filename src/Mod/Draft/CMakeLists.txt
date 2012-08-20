
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
    importDXF.py
    importOCA.py
    importSVG.py
    importAirfoilDAT.py
    macros.py
    Draft_rc.py
)
SOURCE_GROUP("" FILES ${Draft_SRCS})

SET(DraftLibs_SRCS
    draftlibs/dxfColorMap.py
    draftlibs/dxfImportObjects.py
    draftlibs/dxfLibrary.py
    draftlibs/dxfReader.py
    draftlibs/__init__.py
)
SOURCE_GROUP("draftlibs" FILES ${DraftLibs_SRCS})

SET(all_files ${Draft_SRCS} ${DraftLibs_SRCS})

ADD_CUSTOM_TARGET(Draft ALL
    SOURCES ${all_files}
)

fc_copy_sources(Draft "${CMAKE_BINARY_DIR}/Mod/Draft" ${all_files})


INSTALL(
    FILES
        ${DraftLibs_SRCS}
    DESTINATION
        Mod/Draft/draftlibs
)
INSTALL(
    FILES
        ${Draft_SRCS}
    DESTINATION
        Mod/Draft
)
