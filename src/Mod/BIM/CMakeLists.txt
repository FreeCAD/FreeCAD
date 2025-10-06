find_package(PythonInterp REQUIRED)

# ==============================================================================
# File Management
# Define the source files and resources of the BIM workbench
# ==============================================================================

# If we are building the GUI mode, we need to process the Qt resource file
# Note that if any of the files listed in the .qrc file changes, the
# resource file will NOT be regenerated automatically. To work around this,
# use `touch` on the .qrc file to force regeneration.
IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Arch_QRC_SRCS Resources/Arch.qrc)
ENDIF (BUILD_GUI)

SET(Arch_SRCS
    Init.py
    InitGui.py
    ArchComponent.py
    ArchIFC.py
    ArchIFCView.py
    ArchIFCSchema.py
    ArchProject.py
    ArchWall.py
    Arch.py
    ArchSite.py
    ArchStructure.py
    ArchCommands.py
    ArchSectionPlane.py
    ArchWindow.py
    ArchWindowPresets.py
    ArchAxis.py
    ArchAxisSystem.py
    ArchGrid.py
    ArchVRM.py
    ArchRoof.py
    ArchStairs.py
    ArchSpace.py
    ArchRebar.py
    ArchFrame.py
    ArchPanel.py
    ArchEquipment.py
    ArchCutPlane.py
    ArchMaterial.py
    ArchSchedule.py
    ArchProfile.py
    ArchPrecast.py
    ArchPipe.py
    ArchNesting.py
    ArchBuildingPart.py
    ArchReference.py
    ArchFence.py
    OfflineRenderingUtils.py
    ArchTruss.py
    ArchCurtainWall.py
    ArchSketchObject.py
    BimSelect.py
    BimStatus.py
    TestArch.py
    TestArchGui.py
    ArchReport.py
    ArchSql.py
)

SET(importers_SRCS
    importers/__init__.py
    importers/importIFC.py
    importers/importIFClegacy.py
    importers/importIFCHelper.py
    importers/importIFCmulticore.py
    importers/importDAE.py
    importers/importOBJ.py
    importers/importWebGL.py
    importers/importJSON.py
    importers/importSH3D.py
    importers/importSH3DHelper.py
    importers/import3DS.py
    importers/importSHP.py
    importers/importGBXML.py
    importers/exportIFCStructuralTools.py
    importers/exportIFC.py
    importers/exportIFCHelper.py
    importers/samples/Sample.sh3d
)

SET(Dice3DS_SRCS
    Dice3DS/__init__.py
    Dice3DS/util.py
    Dice3DS/dom3ds.py
)

SET(Arch_presets
    Presets/profiles.csv
    Presets/pset_definitions.csv
    Presets/qto_definitions.csv
    Presets/ifc_products_IFC2X3.json
    Presets/ifc_products_IFC4.json
    Presets/ifc_types_IFC2X3.json
    Presets/ifc_types_IFC4.json
    Presets/ifc_contexts_IFC2X3.json
    Presets/ifc_contexts_IFC4.json
    Presets/properties_conversion.csv
    Presets/ArchReport/QueryPresets/all_spaces.json
    Presets/ArchReport/QueryPresets/all_walls.json
    Presets/ArchReport/QueryPresets/count_by_ifc_class.json
    Presets/ArchReport/ReportPresets/room_schedule.json
    Presets/ArchReport/ReportPresets/wall_quantities.json
)

SET(bimcommands_SRCS
    bimcommands/BimArchUtils.py
    bimcommands/BimAxis.py
    bimcommands/BimBackground.py
    bimcommands/BimBeam.py
    bimcommands/BimBox.py
    bimcommands/BimBuilder.py
    bimcommands/BimBuildingPart.py
    bimcommands/BimClassification.py
    bimcommands/BimClone.py
    bimcommands/BimColumn.py
    bimcommands/BimCommon.py
    bimcommands/BimCompound.py
    bimcommands/BimConvert.py
    bimcommands/BimCopy.py
    bimcommands/BimCurtainwall.py
    bimcommands/BimCutPlane.py
    bimcommands/BimCut.py
    bimcommands/BimDiff.py
    bimcommands/BimDimensions.py
    bimcommands/BimDoor.py
    bimcommands/BimDrawingView.py
    bimcommands/BimEmptyTrash.py
    bimcommands/BimEquipment.py
    bimcommands/BimExamples.py
    bimcommands/BimExtrude.py
    bimcommands/BimFence.py
    bimcommands/BimFrame.py
    bimcommands/BimFuse.py
    bimcommands/BimGlue.py
    bimcommands/BimHelp.py
    bimcommands/BimIfcElements.py
    bimcommands/BimIfcExplorer.py
    bimcommands/BimIfcProperties.py
    bimcommands/BimIfcQuantities.py
    bimcommands/BimImagePlane.py
    bimcommands/BimLayers.py
    bimcommands/BimLeader.py
    bimcommands/BimLibrary.py
    bimcommands/BimMaterial.py
    bimcommands/BimMoveView.py
    bimcommands/BimNudge.py
    bimcommands/BimOffset.py
    bimcommands/BimPanel.py
    bimcommands/BimPipe.py
    bimcommands/BimPreflight.py
    bimcommands/BimProfile.py
    bimcommands/BimProjectManager.py
    bimcommands/BimProject.py
    bimcommands/BimRebar.py
    bimcommands/BimReextrude.py
    bimcommands/BimReference.py
    bimcommands/BimReorder.py
    bimcommands/BimResetCloneColors.py
    bimcommands/BimRewire.py
    bimcommands/BimRoof.py
    bimcommands/BimSchedule.py
    bimcommands/BimSectionPlane.py
    bimcommands/BimSetup.py
    bimcommands/BimShape2DView.py
    bimcommands/BimSimpleCopy.py
    bimcommands/BimSite.py
    bimcommands/BimSketch.py
    bimcommands/BimSlab.py
    bimcommands/BimSpace.py
    bimcommands/BimStairs.py
    bimcommands/BimTDPage.py
    bimcommands/BimTDView.py
    bimcommands/BimText.py
    bimcommands/BimTogglePanels.py
    bimcommands/BimTrash.py
    bimcommands/BimTruss.py
    bimcommands/BimTutorial.py
    bimcommands/BimUnclone.py
    bimcommands/BimUngroup.py
    bimcommands/BimViews.py
    bimcommands/BimWall.py
    bimcommands/BimWelcome.py
    bimcommands/BimWindow.py
    bimcommands/BimWindows.py
    bimcommands/BimWPCommands.py
    bimcommands/BimReport.py
    bimcommands/__init__.py
)

SET(BIM_templates
    Resources/templates/webgl_export_template.html
)

SET(nativeifc_SRCS
    nativeifc/ifc_commands.py
    nativeifc/ifc_diff.py
    nativeifc/ifc_generator.py
    nativeifc/ifc_geometry.py
    nativeifc/ifc_import.py
    nativeifc/ifc_layers.py
    nativeifc/ifc_materials.py
    nativeifc/ifc_objects.py
    nativeifc/ifc_observer.py
    nativeifc/ifc_performance_test.py
    nativeifc/ifc_psets.py
    nativeifc/ifc_selftest.py
    nativeifc/ifc_status.py
    nativeifc/ifc_tools.py
    nativeifc/ifc_tree.py
    nativeifc/ifc_viewproviders.py
    nativeifc/__init__.py
    nativeifc/ifc_openshell.py
    nativeifc/ifc_types.py
    nativeifc/ifc_export.py
    nativeifc/ifc_classification.py
)

SET(bimtests_SRCS
    bimtests/TestArchAxis.py
    bimtests/TestArchBase.py
    bimtests/TestArchBaseGui.py
    bimtests/TestArchComponent.py
    bimtests/TestArchBuildingPart.py
    bimtests/TestArchRoof.py
    bimtests/TestArchSpace.py
    bimtests/TestArchWall.py
    bimtests/TestArchMaterial.py
    bimtests/TestArchPanel.py
    bimtests/TestArchWindow.py
    bimtests/TestArchStairs.py
    bimtests/TestArchStructure.py
    bimtests/TestArchPipe.py
    bimtests/TestArchCurtainWall.py
    bimtests/TestArchProfile.py
    bimtests/TestArchProject.py
    bimtests/TestArchSectionPlane.py
    bimtests/TestArchRebar.py
    bimtests/TestArchGrid.py
    bimtests/TestArchFence.py
    bimtests/TestArchEquipment.py
    bimtests/TestArchFrame.py
    bimtests/TestArchReference.py
    bimtests/TestArchSchedule.py
    bimtests/TestArchSiteGui.py
    bimtests/TestArchTruss.py
    bimtests/TestWebGLExport.py
    bimtests/TestWebGLExportGui.py
    bimtests/TestArchImportersGui.py
    bimtests/TestArchBuildingPartGui.py
    bimtests/TestArchReport.py
    bimtests/TestArchReportGui.py
)

set(bimtests_FIXTURES
    bimtests/fixtures/FC_site_simple-102.FCStd
    bimtests/fixtures/__init__.py
    bimtests/fixtures/BimFixtures.py
)

SET(BIMGuiIcon_SVG
    Resources/icons/BIMWorkbench.svg
)

SET(ImportersSample_Files
    importers/samples/Sample.sh3d
)

# TODO:
# - ImportersSample_Files should probably be merged into bimtests_FIXTURES
# - BIM_templates should probably be merged into Arch_presets

SET(ArchSql_resources
    Resources/ArchSql.lark
    Resources/ArchSqlParserGenerator.py
)

set(BIM_LARK_GRAMMAR ${CMAKE_CURRENT_SOURCE_DIR}/Resources/ArchSql.lark)
set(BIM_PARSER_GENERATOR ${CMAKE_CURRENT_SOURCE_DIR}/Resources/ArchSqlParserGenerator.py)

set(BIM_GENERATED_PARSER ${CMAKE_BINARY_DIR}/Mod/BIM/generated_sql_parser.py)

add_custom_command(
    OUTPUT ${BIM_GENERATED_PARSER}
    COMMAND ${PYTHON_EXECUTABLE} ${BIM_PARSER_GENERATOR} ${BIM_LARK_GRAMMAR} ${BIM_GENERATED_PARSER}
    DEPENDS ${BIM_LARK_GRAMMAR} ${BIM_PARSER_GENERATOR}
    COMMENT "Generating Arch SQL parser..."
)

# ==============================================================================
# Developer workflow
# This populates the build tree using the file lists from the Install Manifest
# above, enabling the workbench to be run directly from the build tree.
#
# For a faster development cycle when working on Python or resource
# files, symlinking can be enabled. This allows developers to edit source files
# and see the changes immediately without needing to rebuild the project.
#
# To enable this, the build directory needs to be reconfigured from the command
# line with:
#   cmake -DINSTALL_PREFER_SYMLINKS=ON <path_to_build_dir>
#
# Note for Windows users: "Developer Mode" must be enabled in the
# Windows settings for unprivileged symlink creation to work.
# ==============================================================================

# The `BIM` custom target aggregates BIM module sources for IDEs and the build
# system.
#
# - Listing files here makes IDEs show the workbench and its files under a
#   single node.
# - The ALL keyword makes this target part of the default build (so the build
#   tree is populated automatically).
# - Importantly, since they share the same output files, it makes the
#   `fc_copy_sources` and `fc_target_copy_resource` output a dependency of the
#   BIM target. Thus building the BIM target triggers populating the build tree
#   with the workbench sources and resources, if needed.
ADD_CUSTOM_TARGET(BIM ALL
    SOURCES ${Arch_SRCS}
            ${Arch_QRC_SRCS}
            ${Dice3DS_SRCS}
            ${Arch_presets}
            ${ArchGuiIcon_SVG}
            ${importers_SRCS}
            ${ImportersSample_Files}
            ${bimcommands_SRCS}
            ${bimtests_SRCS}
            ${bimtests_FIXTURES}
            ${nativeifc_SRCS}
            ${BIMGuiIcon_SVG}
            ${BIM_templates}
)

add_custom_target(ArchSqlParser ALL DEPENDS ${BIM_GENERATED_PARSER})

# Populate the build tree with the BIM workbench sources and test data
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${Arch_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${Dice3DS_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${importers_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${bimcommands_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${bimtests_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${bimtests_FIXTURES})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${ImportersSample_Files})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${nativeifc_SRCS})

# Populate the build tree with the BIM workbench resources.
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/BIM" ${BIMGuiIcon_SVG})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/BIM" ${Arch_presets})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/BIM" ${BIM_templates})
fc_copy_sources(BIM "${CMAKE_SOURCE_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/BIM" ${ArchSql_resources})

# For generated resources, we cannot rely on `fc_copy_sources` in case
# INSTALL_PREFER_SYMLINKS=ON has been specified, since we're generating a new
# file not present in the source tree and thus cannot create a symlink to it.
IF (BUILD_GUI)
    fc_target_copy_resource(BIM
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/BIM
        Arch_rc.py)
ENDIF (BUILD_GUI)

# ==============================================================================
# Install Manifest
# Defines what gets installed into the final package, running the --install
# target.
# ==============================================================================

INSTALL(
    FILES
        ${Arch_SRCS}
        ${Arch_QRC_SRCS}
    DESTINATION Mod/BIM
)

INSTALL(
    FILES
        ${Dice3DS_SRCS}
    DESTINATION Mod/BIM/Dice3DS
)

INSTALL(
    FILES
        ${importers_SRCS}
    DESTINATION Mod/BIM/importers
)

INSTALL(
    FILES
        ${bimcommands_SRCS}
    DESTINATION Mod/BIM/bimcommands
)

INSTALL(
    FILES
        ${bimtests_SRCS}
    DESTINATION Mod/BIM/bimtests
)

INSTALL(
    FILES
        ${bimtests_FIXTURES}
    DESTINATION Mod/BIM/bimtests/fixtures
)

INSTALL(
    FILES
        ${nativeifc_SRCS}
    DESTINATION Mod/BIM/nativeifc
)

INSTALL(
    DIRECTORY
        Presets
    DESTINATION
        "${CMAKE_INSTALL_DATADIR}/Mod/BIM"
)

INSTALL(
    FILES
        ${BIMGuiIcon_SVG}
    DESTINATION
       "${CMAKE_INSTALL_DATADIR}/Mod/BIM/Resources/icons"
)

INSTALL(
    FILES
        ${BIM_templates}
    DESTINATION
        "${CMAKE_INSTALL_DATADIR}/Mod/BIM/Resources/templates"
    )

INSTALL(
    FILES
        ${BIM_GENERATED_PARSER}
    DESTINATION
        Mod/BIM
)
