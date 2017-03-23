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
        TestPathApp.py

    DESTINATION
        Mod/Path
)

SET(PathScripts_SRCS
    PathCommands.py
    PathScripts/PathAreaUtils.py
    PathScripts/PathArray.py
    PathScripts/PathComment.py
    PathScripts/PathCompoundExtended.py
    PathScripts/PathContour.py
    PathScripts/PathCopy.py
    PathScripts/PathCustom.py
    PathScripts/PathDressup.py
    PathScripts/PathDressupDogbone.py
    PathScripts/PathDressupDragknife.py
    PathScripts/PathDressupHoldingTags.py
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
    PathScripts/PathLog.py
    PathScripts/PathMillFace.py
    PathScripts/PathPlane.py
    PathScripts/PathPocket.py
    PathScripts/PathPost.py
    PathScripts/PathPostProcessor.py
    PathScripts/PathPreferences.py
    PathScripts/PathPreferencesPathDressup.py
    PathScripts/PathPreferencesPathJob.py
    PathScripts/PathProfile.py
    PathScripts/PathProfileEdges.py
    PathScripts/PathSanity.py
    PathScripts/PathSelection.py
    PathScripts/PathSimpleCopy.py
    PathScripts/PathStock.py
    PathScripts/PathStop.py
    PathScripts/PathHelix.py
    PathScripts/kdtree.py
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
    PathScripts/linuxcnc_post.py
    PathScripts/opensbp_post.py
    PathScripts/opensbp_pre.py
    PathScripts/phillips_post.py
    PathScripts/rml_post.py
    PathScripts/slic3r_pre.py
    PathScripts/smoothie_post.py
)

SET(PathScripts_NC_SRCS
    PathScripts/nc/nc.py
    PathScripts/nc/iso.py
    PathScripts/nc/__init__.py
    PathScripts/nc/format.py
    PathScripts/nc/iso_codes.py
)

SET(PathTests_SRCS
    PathTests/PathTestUtils.py
    PathTests/TestPathCore.py
    PathTests/TestPathDepthParams.py
    PathTests/TestPathDressupHoldingTags.py
    PathTests/TestPathGeom.py
    PathTests/TestPathLog.py
    PathTests/TestPathPost.py
    PathTests/__init__.py
    PathTests/test_linuxcnc_00.ngc
)

SET(all_files
    ${PathScripts_SRCS}
    ${PathScripts_NC_SRCS}
)

ADD_CUSTOM_TARGET(PathScripts ALL
    SOURCES ${all_files}
)

SET(test_files
  TestPathApp.py
  ${PathTests_SRCS}
)

ADD_CUSTOM_TARGET(PathTests ALL
    SOURCES ${test_files}
)

fc_copy_sources(PathScripts "${CMAKE_BINARY_DIR}/Mod/Path" ${all_files})
fc_copy_sources(PathTests "${CMAKE_BINARY_DIR}/Mod/Path" ${test_files})

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

INSTALL(
    FILES
        ${PathTests_SRCS}
    DESTINATION
        Mod/Path/PathTests
)

