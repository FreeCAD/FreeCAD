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
    PathMachineState.py
    PathFeedRate.py
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
    PathScripts/drillableLib.py
    PathScripts/PathAdaptive.py
    PathScripts/PathAdaptiveGui.py
    PathScripts/PathAreaOp.py
    PathScripts/PathArray.py
    PathScripts/PathCircularHoleBase.py
    PathScripts/PathCircularHoleBaseGui.py
    PathScripts/PathCamoticsGui.py
    PathScripts/PathComment.py
    PathScripts/PathCopy.py
    PathScripts/PathCustom.py
    PathScripts/PathCustomGui.py
    PathScripts/PathDeburr.py
    PathScripts/PathDeburrGui.py
    PathScripts/PathDressup.py
    PathScripts/PathDressupAxisMap.py
    PathScripts/PathDressupDogbone.py
    PathScripts/PathDressupDragknife.py
    PathScripts/PathDressupHoldingTags.py
    PathScripts/PathDressupLeadInOut.py
    PathScripts/PathDressupPathBoundary.py
    PathScripts/PathDressupPathBoundaryGui.py
    PathScripts/PathDressupRampEntry.py
    PathScripts/PathDressupTag.py
    PathScripts/PathDressupTagGui.py
    PathScripts/PathDressupTagPreferences.py
    PathScripts/PathDressupZCorrect.py
    PathScripts/PathDrilling.py
    PathScripts/PathDrillingGui.py
    PathScripts/PathEngrave.py
    PathScripts/PathEngraveBase.py
    PathScripts/PathEngraveGui.py
    PathScripts/PathFeatureExtensions.py
    PathScripts/PathFeatureExtensionsGui.py
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
    PathScripts/PathPreferencesAdvanced.py
    PathScripts/PathPreferencesPathDressup.py
    PathScripts/PathPreferencesPathJob.py
    PathScripts/PathProbe.py
    PathScripts/PathProbeGui.py
    PathScripts/PathProfile.py
    PathScripts/PathProfileContour.py
    PathScripts/PathProfileContourGui.py
    PathScripts/PathProfileEdges.py
    PathScripts/PathProfileEdgesGui.py
    PathScripts/PathProfileFaces.py
    PathScripts/PathProfileFacesGui.py
    PathScripts/PathProfileGui.py
    PathScripts/PathProperty.py
    PathScripts/PathPropertyBag.py
    PathScripts/PathPropertyBagGui.py
    PathScripts/PathPropertyEditor.py
    PathScripts/PathSanity.py
    PathScripts/PathSelection.py
    PathScripts/PathSetupSheet.py
    PathScripts/PathSetupSheetGui.py
    PathScripts/PathSetupSheetOpPrototype.py
    PathScripts/PathSetupSheetOpPrototypeGui.py
    PathScripts/PathSimpleCopy.py
    PathScripts/PathSimulatorGui.py
    PathScripts/PathSlot.py
    PathScripts/PathSlotGui.py
    PathScripts/PathStock.py
    PathScripts/PathStop.py
    PathScripts/PathSurface.py
    PathScripts/PathSurfaceGui.py
    PathScripts/PathSurfaceSupport.py
    PathScripts/PathThreadMilling.py
    PathScripts/PathThreadMillingGui.py
    PathScripts/PathToolBit.py
    PathScripts/PathToolBitCmd.py
    PathScripts/PathToolBitEdit.py
    PathScripts/PathToolBitGui.py
    PathScripts/PathToolBitLibraryCmd.py
    PathScripts/PathToolBitLibraryGui.py
    PathScripts/PathToolController.py
    PathScripts/PathToolControllerGui.py
    PathScripts/PathToolEdit.py
    PathScripts/PathToolLibraryEditor.py
    PathScripts/PathToolLibraryManager.py
    PathScripts/PathUtil.py
    PathScripts/PathUtils.py
    PathScripts/PathUtilsGui.py
    PathScripts/PathVcarve.py
    PathScripts/PathVcarveGui.py
    PathScripts/PathWaterline.py
    PathScripts/PathWaterlineGui.py
    PathScripts/PostUtils.py
    PathScripts/__init__.py
)

SET(Generator_SRCS
    Generators/drill_generator.py
    Generators/helix_generator.py
    Generators/rotation_generator.py
    Generators/threadmilling_generator.py
    Generators/toolchange_generator.py
)

SET(PathScripts_post_SRCS
    PathScripts/post/__init__.py
    PathScripts/post/centroid_post.py
    PathScripts/post/comparams_post.py
    PathScripts/post/dxf_post.py
    PathScripts/post/dynapath_post.py
    PathScripts/post/example_pre.py
    PathScripts/post/fablin_post.py
    PathScripts/post/fanuc_post.py
    PathScripts/post/gcode_pre.py
    PathScripts/post/grbl_post.py
    PathScripts/post/heidenhain_post.py
    PathScripts/post/jtech_post.py
    PathScripts/post/KineticNCBeamicon2_post.py
    PathScripts/post/linuxcnc_post.py
    PathScripts/post/mach3_mach4_post.py
    PathScripts/post/marlin_post.py
    PathScripts/post/nccad_post.py
    PathScripts/post/opensbp_post.py
    PathScripts/post/opensbp_pre.py
    PathScripts/post/philips_post.py
    PathScripts/post/rml_post.py
    PathScripts/post/rrf_post.py
    PathScripts/post/slic3r_pre.py
    PathScripts/post/smoothie_post.py
    PathScripts/post/uccnc_post.py
)

SET(PathPythonGui_SRCS
    PathPythonGui/__init__.py
    PathPythonGui/simple_edit_panel.py
)

SET(Tools_SRCS
    Tools/README.md
)

SET(Tools_Bit_SRCS
    Tools/Bit/45degree_chamfer.fctb
    Tools/Bit/5mm-thread-cutter.fctb
    Tools/Bit/5mm_Drill.fctb
    Tools/Bit/5mm_Endmill.fctb
    Tools/Bit/60degree_Vbit.fctb
    Tools/Bit/6mm_Ball_End.fctb
    Tools/Bit/6mm_Bullnose.fctb
    Tools/Bit/probe.fctb
    Tools/Bit/slittingsaw.fctb
)

SET(Tools_Library_SRCS
    Tools/Library/Default.fctl
)

SET(Tools_Shape_SRCS
    Tools/Shape/ballend.fcstd
    Tools/Shape/bullnose.fcstd
    Tools/Shape/drill.fcstd
    Tools/Shape/chamfer.fcstd
    Tools/Shape/endmill.fcstd
    Tools/Shape/probe.fcstd
    Tools/Shape/slittingsaw.fcstd
    Tools/Shape/thread-mill.fcstd
    Tools/Shape/v-bit.fcstd
)

SET(PathTests_SRCS
    PathTests/__init__.py
    PathTests/boxtest.fcstd
    PathTests/Drilling_1.FCStd
    PathTests/PathTestUtils.py
    PathTests/test_adaptive.fcstd
    PathTests/test_centroid_00.ngc
    PathTests/test_filenaming.fcstd
    PathTests/test_geomop.fcstd
    PathTests/test_holes00.fcstd
    PathTests/test_linuxcnc_00.ngc
    PathTests/TestPathAdaptive.py
    PathTests/TestPathCore.py
    PathTests/TestPathDeburr.py
    PathTests/TestPathDepthParams.py
    PathTests/TestPathDressupDogbone.py
    PathTests/TestPathDressupHoldingTags.py
    PathTests/TestPathDrillGenerator.py
    PathTests/TestPathDrillable.py
    PathTests/TestPathRotationGenerator.py
    PathTests/TestPathGeom.py
    PathTests/TestPathHelix.py
    PathTests/TestPathHelpers.py
    PathTests/TestPathHelixGenerator.py
    PathTests/TestPathLog.py
    PathTests/TestPathOpTools.py
    PathTests/TestPathPost.py
    PathTests/TestPathPreferences.py
    PathTests/TestPathPropertyBag.py
    PathTests/TestPathSetupSheet.py
    PathTests/TestPathStock.py
    PathTests/TestPathToolChangeGenerator.py
    PathTests/TestPathThreadMilling.py
    PathTests/TestPathThreadMillingGenerator.py
    PathTests/TestPathTool.py
    PathTests/TestPathToolBit.py
    PathTests/TestPathToolController.py
    PathTests/TestPathTooltable.py
    PathTests/TestPathUtil.py
    PathTests/TestPathVcarve.py
    PathTests/TestPathVoronoi.py
    PathTests/Tools/Bit/test-path-tool-bit-bit-00.fctb
    PathTests/Tools/Library/test-path-tool-bit-library-00.fctl
    PathTests/Tools/Shape/test-path-tool-bit-shape-00.fcstd
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

SET(PathData_Threads
    Data/Threads/imperial-external-2A.csv
    Data/Threads/imperial-external-3A.csv
    Data/Threads/imperial-internal-2B.csv
    Data/Threads/imperial-internal-3B.csv
    Data/Threads/metric-external-4G6G.csv
    Data/Threads/metric-external-6G.csv
    Data/Threads/metric-internal-6H.csv
)

SET(Path_Data
    ${PathData_Threads}
)

SET(all_files
    ${PathScripts_SRCS}
    ${Generator_SRCS}
    ${PathScripts_post_SRCS}
    ${PathPythonGui_SRCS}
    ${Tools_SRCS}
    ${Tools_Bit_SRCS}
    ${Tools_Library_SRCS}
    ${Tools_Shape_SRCS}
    ${Path_Images}
    ${Path_Data}
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
        ${Generator_SRCS}
    DESTINATION
        Mod/Path/Generators
)

INSTALL(
    FILES
        ${PathTests_SRCS}
    DESTINATION
        Mod/Path/PathTests
)

INSTALL(
  DIRECTORY
      PathTests/Tools
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
        ${PathPythonGui_SRCS}
    DESTINATION
        Mod/Path/PathPythonGui
)

INSTALL(
    FILES
        ${Tools_SRCS}
    DESTINATION
        Mod/Path/Tools
)

INSTALL(
    FILES
        ${Tools_Bit_SRCS}
    DESTINATION
        Mod/Path/Tools/Bit
)

INSTALL(
    FILES
        ${Tools_Library_SRCS}
    DESTINATION
        Mod/Path/Tools/Library
)

INSTALL(
    FILES
      ${Tools_Shape_SRCS}
    DESTINATION
      Mod/Path/Tools/Shape
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

INSTALL(
    FILES
        ${PathData_Threads}
    DESTINATION
        Mod/Path/Data/Threads
)

