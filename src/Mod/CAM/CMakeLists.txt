add_subdirectory(App)
add_subdirectory(libarea)
add_subdirectory(PathSimulator)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Path_Scripts
    Init.py
    PathCommands.py
    TestCAMApp.py
)

if(BUILD_GUI)
    list (APPEND Path_Scripts InitGui.py)
endif(BUILD_GUI)

INSTALL(
    FILES
        ${Path_Scripts}
    DESTINATION
        Mod/CAM
)

SET(PathPython_SRCS
    Path/__init__.py
    Path/Log.py
    Path/Geom.py
    Path/GuiInit.py
    Path/Preferences.py
)

SET(PathPythonBase_SRCS
    Path/Base/__init__.py
    Path/Base/Drillable.py
    Path/Base/FeedRate.py
    Path/Base/Language.py
    Path/Base/MachineState.py
    Path/Base/Property.py
    Path/Base/PropertyBag.py
    Path/Base/SetupSheet.py
    Path/Base/SetupSheetOpPrototype.py
    Path/Base/Util.py
)

SET(PathPythonBaseGui_SRCS
    Path/Base/Gui/__init__.py
    Path/Base/Gui/GetPoint.py
    Path/Base/Gui/IconViewProvider.py
    Path/Base/Gui/PreferencesAdvanced.py
    Path/Base/Gui/PropertyBag.py
    Path/Base/Gui/PropertyEditor.py
    Path/Base/Gui/SetupSheet.py
    Path/Base/Gui/SetupSheetOpPrototype.py
    Path/Base/Gui/Util.py
)

SET(PathPythonDressup_SRCS
    Path/Dressup/__init__.py
    Path/Dressup/Utils.py
    Path/Dressup/Boundary.py
    Path/Dressup/DogboneII.py
    Path/Dressup/Tags.py
)

SET(PathPythonDressupGui_SRCS
    Path/Dressup/Gui/__init__.py
    Path/Dressup/Gui/AxisMap.py
    Path/Dressup/Gui/Dogbone.py
    Path/Dressup/Gui/DogboneII.py
    Path/Dressup/Gui/Dragknife.py
    Path/Dressup/Gui/LeadInOut.py
    Path/Dressup/Gui/Boundary.py
    Path/Dressup/Gui/Preferences.py
    Path/Dressup/Gui/RampEntry.py
    Path/Dressup/Gui/Tags.py
    Path/Dressup/Gui/TagPreferences.py
    Path/Dressup/Gui/ZCorrect.py
)

SET(PathPythonMain_SRCS
    Path/Main/__init__.py
    Path/Main/Job.py
    Path/Main/Stock.py
)

SET(PathPythonMainGui_SRCS
    Path/Main/Gui/__init__.py
    Path/Main/Gui/Camotics.py
    Path/Main/Gui/Fixture.py
    Path/Main/Gui/Inspect.py
    Path/Main/Gui/Job.py
    Path/Main/Gui/JobCmd.py
    Path/Main/Gui/JobDlg.py
    Path/Main/Gui/PreferencesJob.py
    Path/Main/Gui/SanityCmd.py
    Path/Main/Gui/Simulator.py
    Path/Main/Gui/SimulatorGL.py
)

SET(PathPythonMainSanity_SRCS
    Path/Main/Sanity/Sanity.py
    Path/Main/Sanity/ImageBuilder.py
    Path/Main/Sanity/ReportGenerator.py
    Path/Main/Sanity/HTMLTemplate.py
    Path/Main/Sanity/Sanity_Bulb.svg
    Path/Main/Sanity/Sanity_Caution.svg
    Path/Main/Sanity/Sanity_Note.svg
    Path/Main/Sanity/Sanity_Warning.svg
)

SET(PathPythonTools_SRCS
    Path/Tool/__init__.py
    Path/Tool/Bit.py
    Path/Tool/Controller.py
)

SET(PathPythonToolsGui_SRCS
    Path/Tool/Gui/__init__.py
    Path/Tool/Gui/Bit.py
    Path/Tool/Gui/BitCmd.py
    Path/Tool/Gui/BitEdit.py
    Path/Tool/Gui/BitLibraryCmd.py
    Path/Tool/Gui/BitLibrary.py
    Path/Tool/Gui/Controller.py
)

SET(PathPythonPost_SRCS
    Path/Post/__init__.py
    Path/Post/Command.py
    Path/Post/Processor.py
    Path/Post/Utils.py
    Path/Post/UtilsArguments.py
    Path/Post/UtilsExport.py
    Path/Post/UtilsParse.py
)

SET(PathPythonPostScripts_SRCS
    Path/Post/scripts/__init__.py
    Path/Post/scripts/centroid_post.py
    Path/Post/scripts/comparams_post.py
    Path/Post/scripts/dxf_post.py
    Path/Post/scripts/dynapath_post.py
    Path/Post/scripts/estlcam_post.py
    Path/Post/scripts/example_pre.py
    Path/Post/scripts/fablin_post.py
    Path/Post/scripts/fanuc_post.py
    Path/Post/scripts/fangling_post.py
    Path/Post/scripts/gcode_pre.py
    Path/Post/scripts/generic_post.py
    Path/Post/scripts/grbl_post.py
    Path/Post/scripts/heidenhain_post.py
    Path/Post/scripts/jtech_post.py
    Path/Post/scripts/KineticNCBeamicon2_post.py
    Path/Post/scripts/linuxcnc_post.py
    Path/Post/scripts/mach3_mach4_post.py
    Path/Post/scripts/marlin_post.py
    Path/Post/scripts/nccad_post.py
    Path/Post/scripts/opensbp_post.py
    Path/Post/scripts/opensbp_pre.py
    Path/Post/scripts/philips_post.py
    Path/Post/scripts/refactored_centroid_post.py
    Path/Post/scripts/refactored_grbl_post.py
    Path/Post/scripts/refactored_linuxcnc_post.py
    Path/Post/scripts/refactored_mach3_mach4_post.py
    Path/Post/scripts/refactored_test_post.py
    Path/Post/scripts/rml_post.py
    Path/Post/scripts/rrf_post.py
    Path/Post/scripts/slic3r_pre.py
    Path/Post/scripts/smoothie_post.py
    Path/Post/scripts/uccnc_post.py
    Path/Post/scripts/wedm_post.py
)

SET(PathPythonOp_SRCS
    Path/Op/__init__.py
    Path/Op/Adaptive.py
    Path/Op/Area.py
    Path/Op/Base.py
    Path/Op/CircularHoleBase.py
    Path/Op/Custom.py
    Path/Op/Deburr.py
    Path/Op/Engrave.py
    Path/Op/EngraveBase.py
    Path/Op/FeatureExtension.py
    Path/Op/Drilling.py
    Path/Op/Helix.py
    Path/Op/MillFace.py
    Path/Op/Pocket.py
    Path/Op/PocketBase.py
    Path/Op/PocketShape.py
    Path/Op/Probe.py
    Path/Op/Profile.py
    Path/Op/Slot.py
    Path/Op/Surface.py
    Path/Op/SurfaceSupport.py
    Path/Op/ThreadMilling.py
    Path/Op/Util.py
    Path/Op/Vcarve.py
    Path/Op/Waterline.py
)

SET(PathPythonOpGui_SRCS
    Path/Op/Gui/__init__.py
    Path/Op/Gui/Adaptive.py
    Path/Op/Gui/Array.py
    Path/Op/Gui/Base.py
    Path/Op/Gui/CircularHoleBase.py
    Path/Op/Gui/Comment.py
    Path/Op/Gui/Copy.py
    Path/Op/Gui/Custom.py
    Path/Op/Gui/Deburr.py
    Path/Op/Gui/Drilling.py
    Path/Op/Gui/Engrave.py
    Path/Op/Gui/FeatureExtension.py
    Path/Op/Gui/Helix.py
    Path/Op/Gui/MillFace.py
    Path/Op/Gui/Pocket.py
    Path/Op/Gui/PocketBase.py
    Path/Op/Gui/PocketShape.py
    Path/Op/Gui/Probe.py
    Path/Op/Gui/Profile.py
    Path/Op/Gui/Selection.py
    Path/Op/Gui/SimpleCopy.py
    Path/Op/Gui/Slot.py
    Path/Op/Gui/Stop.py
    Path/Op/Gui/Surface.py
    Path/Op/Gui/ThreadMilling.py
    Path/Op/Gui/Vcarve.py
    Path/Op/Gui/Waterline.py
)

SET(PathScripts_SRCS
    PathScripts/PathUtils.py
    PathScripts/PathUtilsGui.py
    PathScripts/PathPropertyBag.py
    PathScripts/PathPropertyBagGui.py
    PathScripts/__init__.py
)

SET(PathPythonBaseGenerator_SRCS
    Path/Base/Generator/dogboneII.py
    Path/Base/Generator/drill.py
    Path/Base/Generator/helix.py
    Path/Base/Generator/rotation.py
    Path/Base/Generator/threadmilling.py
    Path/Base/Generator/toolchange.py
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
    Tools/Shape/chamfer.fcstd
    Tools/Shape/dovetail.fcstd
    Tools/Shape/drill.fcstd
    Tools/Shape/endmill.fcstd
    Tools/Shape/probe.fcstd
    Tools/Shape/slittingsaw.fcstd
    Tools/Shape/thread-mill.fcstd
    Tools/Shape/v-bit.fcstd
)

SET(Tests_SRCS
    CAMTests/__init__.py
    CAMTests/boxtest.fcstd
    CAMTests/boxtest1.fcstd
    CAMTests/Drilling_1.FCStd
    CAMTests/drill_test1.FCStd
    CAMTests/FilePathTestUtils.py
    CAMTests/PathTestUtils.py
    CAMTests/test_adaptive.fcstd
    CAMTests/test_profile.fcstd
    CAMTests/test_centroid_00.ngc
    CAMTests/test_filenaming.fcstd
    CAMTests/test_geomop.fcstd
    CAMTests/test_holes00.fcstd
    CAMTests/TestCAMSanity.py
    CAMTests/TestCentroidPost.py
    CAMTests/TestGrblPost.py
    CAMTests/TestLinuxCNCPost.py
    CAMTests/TestMach3Mach4Post.py
    CAMTests/TestPathAdaptive.py
    CAMTests/TestPathCore.py
    CAMTests/TestPathDepthParams.py
    CAMTests/TestPathDressupDogbone.py
    CAMTests/TestPathDressupDogboneII.py
    CAMTests/TestPathDressupHoldingTags.py
    CAMTests/TestPathDrillGenerator.py
    CAMTests/TestPathDrillable.py
    CAMTests/TestPathGeneratorDogboneII.py
    CAMTests/TestPathGeom.py
    CAMTests/TestPathHelix.py
    CAMTests/TestPathHelpers.py
    CAMTests/TestPathHelixGenerator.py
    CAMTests/TestPathLanguage.py
    CAMTests/TestPathLog.py
    CAMTests/TestPathOpDeburr.py
    CAMTests/TestPathOpUtil.py
    CAMTests/TestPathPost.py
    CAMTests/TestPathPreferences.py
    CAMTests/TestPathProfile.py
    CAMTests/TestPathPropertyBag.py
    CAMTests/TestPathRotationGenerator.py
    CAMTests/TestPathSetupSheet.py
    CAMTests/TestPathStock.py
    CAMTests/TestPathToolChangeGenerator.py
    CAMTests/TestPathThreadMilling.py
    CAMTests/TestPathThreadMillingGenerator.py
    CAMTests/TestPathToolBit.py
    CAMTests/TestPathToolController.py
    CAMTests/TestPathUtil.py
    CAMTests/TestPathVcarve.py
    CAMTests/TestPathVoronoi.py
    CAMTests/TestRefactoredCentroidPost.py
    CAMTests/TestRefactoredGrblPost.py
    CAMTests/TestRefactoredLinuxCNCPost.py
    CAMTests/TestRefactoredMach3Mach4Post.py
    CAMTests/TestRefactoredTestPost.py
    CAMTests/TestRefactoredTestPostGCodes.py
    CAMTests/TestRefactoredTestPostMCodes.py
    CAMTests/Tools/Bit/test-path-tool-bit-bit-00.fctb
    CAMTests/Tools/Library/test-path-tool-bit-library-00.fctl
    CAMTests/Tools/Shape/test-path-tool-bit-shape-00.fcstd
)

SET(Tests_Fixtures
    CAMTests/Fixtures/OpHelix_v0-21.FCStd
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
    ${PathPython_SRCS}
    ${PathPythonBase_SRCS}
    ${PathPythonBaseGui_SRCS}
    ${PathPythonBaseGenerator_SRCS}
    ${PathPythonDressup_SRCS}
    ${PathPythonDressupGui_SRCS}
    ${PathPythonMain_SRCS}
    ${PathPythonMainGui_SRCS}
    ${PathPythonMainSanity_SRCS}
    ${PathPythonOp_SRCS}
    ${PathPythonOpGui_SRCS}
    ${PathPythonPost_SRCS}
    ${PathPythonPostScripts_SRCS}
    ${PathPythonTools_SRCS}
    ${PathPythonToolsGui_SRCS}
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
  ${Tests_SRCS}
  ${Tests_Fixtures}
)

ADD_CUSTOM_TARGET(Tests ALL
    SOURCES ${test_files}
)

fc_copy_sources(PathScripts "${CMAKE_BINARY_DIR}/Mod/CAM" ${all_files})
fc_copy_sources(Tests "${CMAKE_BINARY_DIR}/Mod/CAM" ${test_files})

INSTALL(
    FILES
        ${PathScripts_SRCS}
    DESTINATION
        Mod/CAM/PathScripts
)

INSTALL(
    FILES
        ${PathPython_SRCS}
    DESTINATION
        Mod/CAM/Path
)

INSTALL(
    FILES
        ${PathPythonBase_SRCS}
    DESTINATION
        Mod/CAM/Path/Base
)

INSTALL(
    FILES
        ${PathPythonBaseGenerator_SRCS}
    DESTINATION
        Mod/CAM/Path/Base/Generator
)

INSTALL(
    FILES
        ${PathPythonBaseGui_SRCS}
    DESTINATION
        Mod/CAM/Path/Base/Gui
)

INSTALL(
    FILES
        ${PathPythonDressup_SRCS}
    DESTINATION
        Mod/CAM/Path/Dressup
)

INSTALL(
    FILES
        ${PathPythonDressupGui_SRCS}
    DESTINATION
        Mod/CAM/Path/Dressup/Gui
)

INSTALL(
    FILES
        ${PathPythonMain_SRCS}
    DESTINATION
        Mod/CAM/Path/Main
)
INSTALL(
    FILES
        ${PathPythonMainGui_SRCS}
    DESTINATION
        Mod/CAM/Path/Main/Gui
)
INSTALL(
    FILES
        ${PathPythonMainSanity_SRCS}
    DESTINATION
        Mod/CAM/Path/Main/Sanity
)

INSTALL(
    FILES
        ${PathPythonOp_SRCS}
    DESTINATION
        Mod/CAM/Path/Op
)
INSTALL(
    FILES
        ${PathPythonOpGui_SRCS}
    DESTINATION
        Mod/CAM/Path/Op/Gui
)

INSTALL(
    FILES
        ${PathPythonPost_SRCS}
    DESTINATION
        Mod/CAM/Path/Post
)

INSTALL(
    FILES
        ${PathPythonPostScripts_SRCS}
    DESTINATION
        Mod/CAM/Path/Post/scripts
)

INSTALL(
    FILES
        ${PathPythonTools_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool
)

INSTALL(
    FILES
        ${PathPythonToolsGui_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/Gui
)

INSTALL(
    FILES
        ${Tests_SRCS}
    DESTINATION
        Mod/CAM/CAMTests
)

INSTALL(
    FILES
        ${Tests_Fixtures}
    DESTINATION
        Mod/CAM/CAMTests/Fixtures
)

INSTALL(
  DIRECTORY
      CAMTests/Tools
  DESTINATION
        Mod/CAM/CAMTests
)


INSTALL(
    FILES
        ${PathPythonGui_SRCS}
    DESTINATION
        Mod/CAM/PathPythonGui
)

INSTALL(
    FILES
        ${Tools_SRCS}
    DESTINATION
        Mod/CAM/Tools
)

INSTALL(
    FILES
        ${Tools_Bit_SRCS}
    DESTINATION
        Mod/CAM/Tools/Bit
)

INSTALL(
    FILES
        ${Tools_Library_SRCS}
    DESTINATION
        Mod/CAM/Tools/Library
)

INSTALL(
    FILES
      ${Tools_Shape_SRCS}
    DESTINATION
      Mod/CAM/Tools/Shape
)

INSTALL(
    FILES
        ${PathImages_Ops}
    DESTINATION
        Mod/CAM/Images/Ops
)

INSTALL(
    FILES
        ${PathImages_Tools}
    DESTINATION
        Mod/CAM/Images/Tools
)

INSTALL(
    FILES
        ${PathData_Threads}
    DESTINATION
        Mod/CAM/Data/Threads
)
