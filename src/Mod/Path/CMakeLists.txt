add_subdirectory(App)
add_subdirectory(libarea)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
    DESTINATION
        Mod/Path
)

SET(PathScripts_SRCS
    PathScripts/__init__.py
    PathScripts/PostUtils.py
    PathScripts/example_pre.py
    PathScripts/opensbp_pre.py
    PathScripts/example_post.py
    PathScripts/linuxcnc_post.py
    PathScripts/centroid_post.py
    PathScripts/comparams_post.py
    PathScripts/dumper_post.py
    PathScripts/rml_post.py
    PathScripts/TooltableEditor.py
    PathScripts/PathProfile.py
    PathScripts/PathPocket.py
    PathScripts/PathDrilling.py
    PathScripts/PathDressup.py
    PathScripts/PathHop.py
    PathScripts/PathUtils.py
    PathScripts/PathSelection.py
    PathScripts/PathFixture.py
    PathScripts/PathCopy.py
    PathScripts/PathCompoundExtended.py
    PathScripts/PathProject.py
    PathScripts/PathToolTableEdit.py
    PathScripts/PathStock.py
    PathScripts/PathPlane.py
    PathScripts/PathPost.py
    PathScripts/PathLoadTool.py
    PathScripts/PathToolLenOffset.py
    PathScripts/PathComment.py
    PathScripts/PathStop.py
    PathScripts/PathMachine.py
    PathScripts/PathFromShape.py
    PathScripts/DlgSettingsPath.ui
    PathScripts/PathKurveUtils.py
    PathScripts/PathKurve.py
    PathScripts/slic3r_pre.py
    PathScripts/PathFaceProfile.py
    PathScripts/PathFacePocket.py
    PathScripts/PathArray.py
    PathScripts/PathCustom.py
    PathScripts/PathInspect.py
    PathScripts/PathSimpleCopy.py
)

ADD_CUSTOM_TARGET(PathScripts ALL
    SOURCES ${PathScripts_SRCS}
)

fc_copy_sources(PathScripts "${CMAKE_BINARY_DIR}/Mod/Path" ${PathScripts_SRCS})

INSTALL(
    FILES
        ${PathScripts_SRCS}
    DESTINATION
        Mod/Path/PathScripts
       )
