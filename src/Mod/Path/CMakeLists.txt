add_subdirectory(App)
add_subdirectory(libarea)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
        PathCommands.py
    DESTINATION
        Mod/Path
)

SET(PathScripts_SRCS
    PathCommands.py
    PathScripts/DogboneDressup.py
    PathScripts/DragknifeDressup.py
    PathScripts/PathAreaUtils.py
    PathScripts/PathArray.py
    PathScripts/PathComment.py
    PathScripts/PathCompoundExtended.py
    PathScripts/PathContour.py
    PathScripts/PathCopy.py
    PathScripts/PathCustom.py
    PathScripts/PathDressup.py
    PathScripts/PathDrilling.py
    PathScripts/PathEngrave.py
    PathScripts/PathFacePocket.py
    PathScripts/PathFaceProfile.py
    PathScripts/PathFixture.py
    PathScripts/PathFromShape.py
    PathScripts/PathGeom.py
    PathScripts/PathHop.py
    PathScripts/PathInspect.py
    PathScripts/PathJob.py
    PathScripts/PathKurveUtils.py
    PathScripts/PathLoadTool.py
    PathScripts/PathMillFace.py
    PathScripts/PathPlane.py
    PathScripts/PathPocket.py
    PathScripts/PathPost.py
    PathScripts/PathPostProcessor.py
    PathScripts/PathPreferences.py
    PathScripts/PathPreferencesPathJob.py
    PathScripts/PathProfile.py
    PathScripts/PathProfileEdges.py
    PathScripts/PathRemote.py
    PathScripts/PathSanity.py
    PathScripts/PathSelection.py
    PathScripts/PathSimpleCopy.py
    PathScripts/PathStock.py
    PathScripts/PathStop.py
    PathScripts/PathSurface.py
    PathScripts/PathToolLenOffset.py
    PathScripts/PathToolLibraryManager.py
    PathScripts/PathUtils.py
    PathScripts/PostUtils.py
    PathScripts/TooltableEditor.py
    PathScripts/__init__.py
    PathScripts/centroid_post.py
    PathScripts/comparams_post.py
    PathScripts/dumper_post.py
    PathScripts/dynapath_post.py
    PathScripts/example_post.py
    PathScripts/example_pre.py
    PathScripts/generic_post.py
    PathScripts/linuxcnc_post.py
    PathScripts/opensbp_post.py
    PathScripts/opensbp_pre.py
    PathScripts/rml_post.py
    PathScripts/slic3r_pre.py
    PathTests/PathTestUtils.py
    PathTests/TestPathGeom.py
    PathTests/TestPathPost.py
    PathTests/__init__.py
    PathTests/test_linuxcnc_00.ngc
    TestPathApp.py
)

SET(PathScripts_NC_SRCS
    PathScripts/nc/nc.py
    PathScripts/nc/iso.py
    PathScripts/nc/__init__.py
    PathScripts/nc/format.py
    PathScripts/nc/iso_codes.py
)

SET(all_files
    ${PathScripts_SRCS}
    ${PathScripts_NC_SRCS}
)

ADD_CUSTOM_TARGET(PathScripts ALL
    SOURCES ${all_files}
)

fc_copy_sources(PathScripts "${CMAKE_BINARY_DIR}/Mod/Path" ${all_files})

INSTALL(
    FILES
        ${PathScripts_SRCS}
    DESTINATION
        Mod/Path/PathScripts
)

INSTALL(
    FILES
        ${PathScripts_NC_SRCS}
    DESTINATION
        Mod/Path/PathScripts/nc
)

