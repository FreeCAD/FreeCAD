# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)
add_subdirectory(libarea)
add_subdirectory(PathSimulator)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Path_Scripts
    Constants.py
    Init.py
    PathCommands.py
    TestCAMApp.py
    TestCAMGui.py
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
    Path/Dressup/Array.py
    Path/Dressup/Boundary.py
    Path/Dressup/DogboneII.py
    Path/Dressup/Tags.py
)

SET(PathPythonDressupGui_SRCS
    Path/Dressup/Gui/__init__.py
    Path/Dressup/Gui/Array.py
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
    Path/Main/Gui/Editor.py
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
    Path/Tool/camassets.py
    Path/Tool/Controller.py
)

SET(PathPythonToolsAssets_SRCS
    Path/Tool/assets/__init__.py
    Path/Tool/assets/asset.py
    Path/Tool/assets/cache.py
    Path/Tool/assets/manager.py
    Path/Tool/assets/serializer.py
    Path/Tool/assets/uri.py
)

SET(PathPythonToolsMigration_SRCS
    Path/Tool/migration/__init__.py
    Path/Tool/migration/migration.py
)

SET(PathPythonToolsAssetsStore_SRCS
    Path/Tool/assets/store/__init__.py
    Path/Tool/assets/store/base.py
    Path/Tool/assets/store/memory.py
    Path/Tool/assets/store/filestore.py
)

SET(PathPythonToolsAssetsUi_SRCS
    Path/Tool/assets/ui/__init__.py
    Path/Tool/assets/ui/filedialog.py
    Path/Tool/assets/ui/preferences.py
    Path/Tool/assets/ui/util.py
)

SET(PathPythonToolsDocObject_SRCS
    Path/Tool/docobject/__init__.py
)

SET(PathPythonToolsDocObjectModels_SRCS
    Path/Tool/docobject/models/__init__.py
    Path/Tool/docobject/models/docobject.py
)

SET(PathPythonToolsDocObjectUi_SRCS
    Path/Tool/docobject/ui/__init__.py
    Path/Tool/docobject/ui/docobject.py
    Path/Tool/docobject/ui/property.py
)

SET(PathPythonToolsGui_SRCS
    Path/Tool/Gui/__init__.py
    Path/Tool/Gui/Controller.py
)

SET(PathPythonMachineUi_SRCS
    Path/Machine/ui/__init__.py
)

SET(PathPythonMachineUiEditor_SRCS
    Path/Machine/ui/editor/machine_editor.py
    Path/Machine/ui/editor/__init__.py
)

SET(PathPythonMachineModels_SRCS
    Path/Machine/models/__init__.py
    Path/Machine/models/machine.py
)

SET(PathPythonToolsToolBit_SRCS
    Path/Tool/toolbit/__init__.py
    Path/Tool/toolbit/util.py
    Path/Tool/toolbit/migration.py
)

SET(PathPythonToolsToolBitMixins_SRCS
    Path/Tool/toolbit/mixins/__init__.py
    Path/Tool/toolbit/mixins/rotary.py
    Path/Tool/toolbit/mixins/cutting.py
)

SET(PathPythonToolsToolBitModels_SRCS
    Path/Tool/toolbit/models/__init__.py
    Path/Tool/toolbit/models/ballend.py
    Path/Tool/toolbit/models/base.py
    Path/Tool/toolbit/models/bullnose.py
    Path/Tool/toolbit/models/chamfer.py
    Path/Tool/toolbit/models/custom.py
    Path/Tool/toolbit/models/dovetail.py
    Path/Tool/toolbit/models/drill.py
    Path/Tool/toolbit/models/endmill.py
    Path/Tool/toolbit/models/radius.py
    Path/Tool/toolbit/models/probe.py
    Path/Tool/toolbit/models/reamer.py
    Path/Tool/toolbit/models/slittingsaw.py
    Path/Tool/toolbit/models/tap.py
    Path/Tool/toolbit/models/taperedballnose.py
    Path/Tool/toolbit/models/threadmill.py
    Path/Tool/toolbit/models/vbit.py
)

SET(PathPythonToolsToolBitSerializers_SRCS
    Path/Tool/toolbit/serializers/__init__.py
    Path/Tool/toolbit/serializers/camotics.py
    Path/Tool/toolbit/serializers/fctb.py
    Path/Tool/toolbit/serializers/yaml.py
)

SET(PathPythonToolsToolBitUi_SRCS
    Path/Tool/toolbit/ui/__init__.py
    Path/Tool/toolbit/ui/editor.py
    Path/Tool/toolbit/ui/cmd.py
    Path/Tool/toolbit/ui/browser.py
    Path/Tool/toolbit/ui/file.py
    Path/Tool/toolbit/ui/panel.py
    Path/Tool/toolbit/ui/selector.py
    Path/Tool/toolbit/ui/tablecell.py
    Path/Tool/toolbit/ui/toollist.py
    Path/Tool/toolbit/ui/util.py
    Path/Tool/toolbit/ui/view.py
)

SET(PathPythonToolsLibrary_SRCS
    Path/Tool/library/__init__.py
    Path/Tool/library/util.py
)

SET(PathPythonToolsLibraryModels_SRCS
    Path/Tool/library/models/__init__.py
    Path/Tool/library/models/library.py
)

SET(PathPythonToolsLibrarySerializers_SRCS
    Path/Tool/library/serializers/__init__.py
    Path/Tool/library/serializers/fctl.py
    Path/Tool/library/serializers/camotics.py
    Path/Tool/library/serializers/linuxcnc.py
)

SET(PathPythonToolsLibraryUi_SRCS
    Path/Tool/library/ui/__init__.py
    Path/Tool/library/ui/browser.py
    Path/Tool/library/ui/cmd.py
    Path/Tool/library/ui/dock.py
    Path/Tool/library/ui/editor.py
    Path/Tool/library/ui/properties.py
)

SET(PathPythonToolsShape_SRCS
    Path/Tool/shape/__init__.py
    Path/Tool/shape/util.py
    Path/Tool/shape/doc.py
)

SET(PathPythonToolsShapeModels_SRCS
    Path/Tool/shape/models/__init__.py
    Path/Tool/shape/models/ballend.py
    Path/Tool/shape/models/base.py
    Path/Tool/shape/models/bullnose.py
    Path/Tool/shape/models/chamfer.py
    Path/Tool/shape/models/custom.py
    Path/Tool/shape/models/dovetail.py
    Path/Tool/shape/models/drill.py
    Path/Tool/shape/models/endmill.py
    Path/Tool/shape/models/radius.py
    Path/Tool/shape/models/icon.py
    Path/Tool/shape/models/probe.py
    Path/Tool/shape/models/reamer.py
    Path/Tool/shape/models/slittingsaw.py
    Path/Tool/shape/models/tap.py
    Path/Tool/shape/models/taperedballnose.py
    Path/Tool/shape/models/threadmill.py
    Path/Tool/shape/models/vbit.py
)

SET(PathPythonToolsShapeUi_SRCS
    Path/Tool/shape/ui/__init__.py
    Path/Tool/shape/ui/flowlayout.py
    Path/Tool/shape/ui/shapebutton.py
    Path/Tool/shape/ui/shapeselector.py
    Path/Tool/shape/ui/shapewidget.py
)

SET(PathPythonPost_SRCS
    Path/Post/__init__.py
    Path/Post/Command.py
    Path/Post/PostList.py
    Path/Post/Processor.py
    Path/Post/Utils.py
    Path/Post/UtilsArguments.py
    Path/Post/UtilsExport.py
    Path/Post/UtilsParse.py
)

SET(PathPythonPostScripts_SRCS
    Path/Post/scripts/__init__.py
    Path/Post/scripts/centroid_post.py
    Path/Post/scripts/centroid_legacy_post.py
    Path/Post/scripts/dxf_post.py
    Path/Post/scripts/dynapath_post.py
    Path/Post/scripts/dynapath_4060_post.py
    Path/Post/scripts/estlcam_post.py
    Path/Post/scripts/fablin_post.py
    Path/Post/scripts/fanuc_post.py
    Path/Post/scripts/fangling_post.py
    Path/Post/scripts/gcode_pre.py
    Path/Post/scripts/generic_post.py
    Path/Post/scripts/grbl_legacy_post.py
    Path/Post/scripts/heidenhain_post.py
    Path/Post/scripts/jtech_post.py
    Path/Post/scripts/KineticNCBeamicon2_post.py
    Path/Post/scripts/linuxcnc_post.py
    Path/Post/scripts/linuxcnc_legacy_post.py
    Path/Post/scripts/mach3_mach4_post.py
    Path/Post/scripts/mach3_mach4_legacy_post.py
    Path/Post/scripts/masso_g3_post.py
    Path/Post/scripts/marlin_post.py
    Path/Post/scripts/nccad_post.py
    Path/Post/scripts/opensbp_post.py
    Path/Post/scripts/opensbp_pre.py
    Path/Post/scripts/philips_post.py
    Path/Post/scripts/grbl_post.py
    Path/Post/scripts/grbl_legacy_post.py
    Path/Post/scripts/test_post.py
    Path/Post/scripts/rml_post.py
    Path/Post/scripts/rrf_post.py
    Path/Post/scripts/slic3r_pre.py
    Path/Post/scripts/smoothie_post.py
    Path/Post/scripts/snapmaker_post.py
    Path/Post/scripts/svg_post.py
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
    Path/Op/MillFacing.py
    Path/Op/Pocket.py
    Path/Op/PocketBase.py
    Path/Op/PocketShape.py
    Path/Op/Probe.py
    Path/Op/Profile.py
    Path/Op/Slot.py
    Path/Op/Surface.py
    Path/Op/SurfaceSupport.py
    Path/Op/Tapping.py
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
    Path/Op/Gui/MillFacing.py
    Path/Op/Gui/PathShapeTC.py
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
    Path/Op/Gui/Tapping.py
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
    Path/Base/Generator/facing_common.py
    Path/Base/Generator/spiral_facing.py
    Path/Base/Generator/zigzag_facing.py
    Path/Base/Generator/directional_facing.py
    Path/Base/Generator/bidirectional_facing.py
    Path/Base/Generator/helix.py
    Path/Base/Generator/linking.py
    Path/Base/Generator/rotation.py
    Path/Base/Generator/tapping.py
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
    Tools/Bit/30degree_Vbit.fctb
    Tools/Bit/3.175mm_Endmill.fctb
    Tools/Bit/375-16_Tap.fctb
    Tools/Bit/45degree_chamfer.fctb
    Tools/Bit/45degree_Vbit.fctb
    Tools/Bit/5mm-thread-cutter.fctb
    Tools/Bit/5mm_Drill.fctb
    Tools/Bit/5mm_Endmill.fctb
    Tools/Bit/60degree_Vbit.fctb
    Tools/Bit/6mm_Ball_End.fctb
    Tools/Bit/6mm_Bullnose.fctb
    Tools/Bit/90degree_Vbit.fctb
    Tools/Bit/M8x1.25_Tap.fctb
    Tools/Bit/probe.fctb
    Tools/Bit/slittingsaw.fctb
)

SET(Tools_Library_SRCS
    Tools/Library/Default.fctl
)

SET(Tools_Shape_SRCS
    Tools/Shape/ballend.fcstd
    Tools/Shape/ballend.svg
    Tools/Shape/bullnose.fcstd
    Tools/Shape/bullnose.svg
    Tools/Shape/chamfer.fcstd
    Tools/Shape/chamfer.svg
    Tools/Shape/dovetail.fcstd
    Tools/Shape/dovetail.svg
    Tools/Shape/drill.fcstd
    Tools/Shape/drill.svg
    Tools/Shape/endmill.fcstd
    Tools/Shape/endmill.svg
    Tools/Shape/radius.fcstd
    Tools/Shape/radius.svg
    Tools/Shape/probe.fcstd
    Tools/Shape/probe.svg
    Tools/Shape/reamer.fcstd
    Tools/Shape/reamer.svg
    Tools/Shape/slittingsaw.fcstd
    Tools/Shape/slittingsaw.svg
    Tools/Shape/tap.fcstd
    Tools/Shape/tap.svg
    Tools/Shape/taperedballnose.fcstd
    Tools/Shape/taperedballnose.svg
    Tools/Shape/thread-mill.fcstd
    Tools/Shape/thread-mill.svg
    Tools/Shape/v-bit.fcstd
    Tools/Shape/v-bit.svg
)

SET(Tests_SRCS
    CAMTests/__init__.py
    CAMTests/boxtest.fcstd
    CAMTests/dressuptest.FCStd
    CAMTests/Drilling_1.FCStd
    CAMTests/FilePathTestUtils.py
    CAMTests/PathTestUtils.py
    CAMTests/PostTestMocks.py
    CAMTests/test_adaptive.fcstd
    CAMTests/test_profile.fcstd
    CAMTests/test_geomop.fcstd
    CAMTests/test_holes00.fcstd
    CAMTests/TestCAMSanity.py
    CAMTests/TestCentroidPost.py
    CAMTests/TestCentroidLegacyPost.py
    CAMTests/TestFanucPost.py
    CAMTests/TestGenericPost.py
    CAMTests/TestGrblPost.py
    CAMTests/TestGrblLegacyPost.py
    CAMTests/TestLinuxCNCPost.py
    CAMTests/TestLinkingGenerator.py
    CAMTests/TestMachine.py
    CAMTests/TestMach3Mach4Post.py
    CAMTests/TestMach3Mach4LegacyPost.py
    CAMTests/TestMassoG3Post.py
    CAMTests/TestPathAdaptive.py
    CAMTests/TestPathCommandAnnotations.py
    CAMTests/TestPathCore.py
    CAMTests/TestPathDepthParams.py
    CAMTests/TestPathDressupArray.py
    CAMTests/TestPathDressupDogbone.py
    CAMTests/TestPathDressupDogboneII.py
    CAMTests/TestPathDressupHoldingTags.py
    CAMTests/TestPathDrillGenerator.py
    CAMTests/TestPathDrillable.py
    CAMTests/TestPathFacingGenerator.py
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
    CAMTests/TestPathTapGenerator.py
    CAMTests/TestPathToolChangeGenerator.py
    CAMTests/TestPathThreadMilling.py
    CAMTests/TestPathThreadMillingGenerator.py
    CAMTests/TestPathToolAsset.py
    CAMTests/TestPathToolAssetCache.py
    CAMTests/TestPathToolAssetUri.py
    CAMTests/TestPathToolAssetStore.py
    CAMTests/TestPathToolAssetManager.py
    CAMTests/TestPathToolBit.py
    CAMTests/TestPathToolBitSerializer.py
    CAMTests/TestPathToolBitBrowserWidget.py
    CAMTests/TestPathToolBitEditorWidget.py
    CAMTests/TestPathToolBitListWidget.py
    CAMTests/TestPathToolBitPropertyEditorWidget.py
    CAMTests/TestPathToolDocumentObjectEditorWidget.py
    CAMTests/TestPathToolShapeClasses.py
    CAMTests/TestPathToolShapeDoc.py
    CAMTests/TestPathToolShapeIcon.py
    CAMTests/TestPathToolLibrary.py
    CAMTests/TestPathToolLibrarySerializer.py
    CAMTests/TestPathToolController.py
    CAMTests/TestPathUtil.py
    CAMTests/TestPathVcarve.py
    CAMTests/TestPathVoronoi.py
    CAMTests/TestGrblLegacyPost.py
    CAMTests/TestLinuxCNCLegacyPost.py
    CAMTests/TestDressupPost.py
    CAMTests/TestTestPost.py
    CAMTests/TestPostGCodes.py
    CAMTests/TestPostMCodes.py
    CAMTests/TestSnapmakerPost.py
    CAMTests/TestTSPSolver.py
    CAMTests/Tools/Bit/test-path-tool-bit-bit-00.fctb
    CAMTests/Tools/Library/test-path-tool-bit-library-00.fctl
    CAMTests/Tools/Shape/test-path-tool-bit-shape-00.fcstd
    CAMTests/TestToolBitRecomputeState.py
)

SET(Tests_Fixtures
    CAMTests/Fixtures/OpHelix_v0-21.FCStd
)

SET(PathImages_Ops
    Images/Ops/chamfer.svg
)

SET(Path_Images
    ${PathImages_Ops}
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
    ${PathPythonToolsAssets_SRCS}
    ${PathPythonToolsMigration_SRCS}
    ${PathPythonToolsAssetsStore_SRCS}
    ${PathPythonToolsAssetsUi_SRCS}
    ${PathPythonToolsDocObject_SRCS}
    ${PathPythonToolsDocObjectModels_SRCS}
    ${PathPythonToolsDocObjectUi_SRCS}
    ${PathPythonToolsGui_SRCS}
    ${PathPythonToolsShape_SRCS}
    ${PathPythonToolsShapeModels_SRCS}
    ${PathPythonToolsShapeUi_SRCS}
    ${PathPythonToolsToolBit_SRCS}
    ${PathPythonToolsToolBitMixins_SRCS}
    ${PathPythonToolsToolBitModels_SRCS}
    ${PathPythonToolsToolBitSerializers_SRCS}
    ${PathPythonToolsToolBitUi_SRCS}
    ${PathPythonToolsLibrary_SRCS}
    ${PathPythonToolsLibraryModels_SRCS}
    ${PathPythonToolsLibrarySerializers_SRCS}
    ${PathPythonToolsLibraryUi_SRCS}
    ${PathPythonMachineModels_SRCS}
    ${PathPythonMachineUi_SRCS}
    ${PathPythonMachineUiEditor_SRCS}
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
        ${PathPythonToolsAssets_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/assets
)

INSTALL(
    FILES
        ${PathPythonToolsMigration_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/migration
)

INSTALL(
    FILES
        ${PathPythonToolsAssetsStore_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/assets/store
)

INSTALL(
    FILES
        ${PathPythonToolsAssetsUi_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/assets/ui
)

INSTALL(
    FILES
        ${PathPythonToolsGui_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/Gui
)

INSTALL(
    FILES
        ${PathPythonToolsDocObject_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/docobject
)

INSTALL(
    FILES
        ${PathPythonToolsDocObjectModels_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/docobject/models
)

INSTALL(
    FILES
        ${PathPythonToolsDocObjectUi_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/docobject/ui
)

INSTALL(
    FILES
        ${PathPythonToolsShape_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/shape
)

INSTALL(
    FILES
        ${PathPythonToolsShapeUi_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/shape/ui
)

INSTALL(
    FILES
        ${PathPythonToolsShapeModels_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/shape/models
)

INSTALL(
    FILES
        ${PathPythonToolsToolBit_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/toolbit
)

INSTALL(
    FILES
        ${PathPythonToolsToolBitMixins_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/toolbit/mixins
)

INSTALL(
    FILES
        ${PathPythonToolsToolBitModels_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/toolbit/models
)

INSTALL(
    FILES
        ${PathPythonToolsToolBitSerializers_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/toolbit/serializers
)

INSTALL(
    FILES
        ${PathPythonToolsToolBitUi_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/toolbit/ui
)

INSTALL(
    FILES
        ${PathPythonToolsLibrary_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/library
)

INSTALL(
    FILES
        ${PathPythonToolsLibraryModels_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/library/models
)

INSTALL(
    FILES
        ${PathPythonToolsLibrarySerializers_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/library/serializers
)

INSTALL(
    FILES
        ${PathPythonToolsLibraryUi_SRCS}
    DESTINATION
        Mod/CAM/Path/Tool/library/ui
)

INSTALL(
    FILES
        ${PathPythonMachineModels_SRCS}
    DESTINATION
        Mod/CAM/Path/Machine/models
)

INSTALL(
    FILES
        ${PathPythonMachineUi_SRCS}
    DESTINATION
        Mod/CAM/Path/Machine/ui
)

INSTALL(
    FILES
        ${PathPythonMachineUiEditor_SRCS}
    DESTINATION
        Mod/CAM/Path/Machine/ui/editor
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
        ${PathData_Threads}
    DESTINATION
        Mod/CAM/Data/Threads
)
