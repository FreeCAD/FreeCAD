add_subdirectory(App)
add_subdirectory(libarea)
add_subdirectory(PathSimulator)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Path_Scripts
    Init.py
    PathCommands.py
    TestPathApp.py
)

if(BUILD_GUI)
    list (APPEND Path_Scripts InitGui.py)
endif(BUILD_GUI)

INSTALL(
    FILES
        ${Path_Scripts}
    DESTINATION
        Mod/Path
)

SET(PathScripts_SRCS
    PathCommands.py
    PathScripts/PathAreaOp.py
    PathScripts/PathArray.py
    PathScripts/PathCircularHoleBase.py
    PathScripts/PathCircularHoleBaseGui.py
    PathScripts/PathComment.py
    PathScripts/PathCopy.py
    PathScripts/PathCustom.py
    PathScripts/PathDeburr.py
    PathScripts/PathDeburrGui.py
    PathScripts/PathDressup.py
    PathScripts/PathDressupAxisMap.py
    PathScripts/PathDressupDogbone.py
    PathScripts/PathDressupDragknife.py
    PathScripts/PathDressupHoldingTags.py
    PathScripts/PathDressupLeadInOut.py
    PathScripts/PathDressupRampEntry.py
    PathScripts/PathDressupTag.py
    PathScripts/PathDressupTagGui.py
    PathScripts/PathDressupTagPreferences.py
    PathScripts/PathDrilling.py
    PathScripts/PathDrillingGui.py
    PathScripts/PathEngrave.py
    PathScripts/PathEngraveBase.py
    PathScripts/PathEngraveGui.py
    PathScripts/PathFixture.py
    PathScripts/PathGeom.py
    PathScripts/PathGetPoint.py
    PathScripts/PathGui.py
    PathScripts/PathGuiInit.py
    PathScripts/PathHelix.py
    PathScripts/PathHelixGui.py
    PathScripts/PathHop.py
    PathScripts/PathIconViewProvider.py
    PathScripts/PathInspect.py
    PathScripts/PathJob.py
    PathScripts/PathJobCmd.py
    PathScripts/PathJobDlg.py
    PathScripts/PathJobGui.py
    PathScripts/PathLog.py
    PathScripts/PathMillFace.py
    PathScripts/PathMillFaceGui.py
    PathScripts/PathOp.py
    PathScripts/PathOpGui.py
    PathScripts/PathOpTools.py
    PathScripts/PathPocket.py
    PathScripts/PathPocketBase.py
    PathScripts/PathPocketBaseGui.py
    PathScripts/PathPocketGui.py
    PathScripts/PathPocketShape.py
    PathScripts/PathPocketShapeGui.py
    PathScripts/PathPost.py
    PathScripts/PathPostProcessor.py
    PathScripts/PathPreferences.py
    PathScripts/PathPreferencesPathDressup.py
    PathScripts/PathPreferencesPathJob.py
    PathScripts/PathProfileBase.py
    PathScripts/PathProfileBaseGui.py
    PathScripts/PathProfileContour.py
    PathScripts/PathProfileContourGui.py
    PathScripts/PathProfileEdges.py
    PathScripts/PathProfileEdgesGui.py
    PathScripts/PathProfileFaces.py
    PathScripts/PathProfileFacesGui.py
    PathScripts/PathSanity.py
    PathScripts/PathSelection.py
    PathScripts/PathSetupSheet.py
    PathScripts/PathSetupSheetGui.py
    PathScripts/PathSetupSheetOpPrototype.py
    PathScripts/PathSetupSheetOpPrototypeGui.py
    PathScripts/PathSimpleCopy.py
    PathScripts/PathStock.py
    PathScripts/PathStop.py
    PathScripts/PathSurface.py
    PathScripts/PathSurfaceGui.py
    PathScripts/PathToolController.py
    PathScripts/PathToolEdit.py
    PathScripts/PathToolLibraryManager.py
    PathScripts/PathUtil.py
    PathScripts/PathUtils.py
    PathScripts/PathSimulatorGui.py
    PathScripts/PostUtils.py
    PathScripts/PathAdaptiveGui.py
    PathScripts/PathAdaptive.py
    PathScripts/__init__.py
)

SET(PathScripts_post_SRCS
    PathScripts/post/__init__.py
    PathScripts/post/centroid_post.py
    PathScripts/post/comparams_post.py
    PathScripts/post/dynapath_post.py
    PathScripts/post/example_pre.py
    PathScripts/post/grbl_post.py
    PathScripts/post/grbl_G81_post.py
    PathScripts/post/linuxcnc_post.py
    PathScripts/post/opensbp_post.py
    PathScripts/post/opensbp_pre.py
    PathScripts/post/philips_post.py
    PathScripts/post/rml_post.py
    PathScripts/post/slic3r_pre.py
    PathScripts/post/smoothie_post.py
)


SET(PathTests_SRCS
    PathTests/__init__.py
    PathTests/PathTestUtils.py
    PathTests/TestPathCore.py
    PathTests/TestPathDeburr.py
    PathTests/TestPathDepthParams.py
    PathTests/TestPathDressupDogbone.py
    PathTests/TestPathDressupHoldingTags.py
    PathTests/TestPathGeom.py
    PathTests/TestPathLog.py
    PathTests/TestPathOpTools.py
    PathTests/TestPathPost.py
    PathTests/TestPathSetupSheet.py
    PathTests/TestPathStock.py
    PathTests/TestPathTool.py
    PathTests/TestPathToolController.py
    PathTests/TestPathTooltable.py
    PathTests/TestPathUtil.py
    PathTests/boxtest.fcstd
    PathTests/test_centroid_00.ngc
    PathTests/test_geomop.fcstd
    PathTests/test_linuxcnc_00.ngc
)

SET(PathImages_Ops
    Images/Ops/chamfer.svg
)

SET(PathImages_Tools
    Images/Tools/drill.svg
    Images/Tools/endmill.svg
    Images/Tools/v-bit.svg
)

SET(Path_Images
    ${PathImages_Ops}
    ${PathImages_Tools}
)

SET(all_files
    ${PathScripts_SRCS}
    ${PathScripts_post_SRCS}
    ${Path_Images}
)

ADD_CUSTOM_TARGET(PathScripts ALL
    SOURCES ${all_files}
)

SET(test_files
  ${Path_Scripts}
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
        ${PathTests_SRCS}
    DESTINATION
        Mod/Path/PathTests
)

INSTALL(
    FILES
        ${PathScripts_post_SRCS}
    DESTINATION
        Mod/Path/PathScripts/post
)

INSTALL(
    FILES
        ${PathImages_Ops}
    DESTINATION
        Mod/Path/Images/Ops
)

INSTALL(
    FILES
        ${PathImages_Tools}
    DESTINATION
        Mod/Path/Images/Tools
)

