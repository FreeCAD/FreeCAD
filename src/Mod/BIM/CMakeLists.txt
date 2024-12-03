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
    TestArch.py
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
    importers/import3DS.py
    importers/importSHP.py
    importers/importGBXML.py
    importers/exportIFCStructuralTools.py
    importers/exportIFC.py
    importers/exportIFCHelper.py
)

SET(Dice3DS_SRCS
    Dice3DS/__init__.py
    Dice3DS/util.py
    Dice3DS/dom3ds.py
)

SET(Arch_presets
    Presets/profiles.csv
    Presets/pset_definitions.csv
    Presets/ifc_products_IFC2X3.json
    Presets/ifc_products_IFC4.json
    Presets/ifc_types_IFC2X3.json
    Presets/ifc_types_IFC4.json
    Presets/ifc_contexts_IFC2X3.json
    Presets/ifc_contexts_IFC4.json
    Presets/properties_conversion.csv
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
    bimcommands/__init__.py
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
)

SOURCE_GROUP("" FILES ${Arch_SRCS})

SET(BIMGuiIcon_SVG
    Resources/icons/BIMWorkbench.svg
)

ADD_CUSTOM_TARGET(BIM ALL
    SOURCES ${Arch_SRCS}
            ${Arch_QRC_SRCS}
            ${Dice3DS_SRCS}
            ${Arch_presets}
            ${ArchGuiIcon_SVG}
            ${importers_SRCS}
            ${bimcommands_SRCS}
            ${nativeifc_SRCS}
            ${BIMGuiIcon_SVG}
)

fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${Arch_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${Dice3DS_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${importers_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${bimcommands_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/Mod/BIM" ${nativeifc_SRCS})
fc_copy_sources(BIM "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/BIM" ${BIMGuiIcon_SVG})

fc_target_copy_resource(BIM
    ${CMAKE_SOURCE_DIR}/src/Mod/BIM
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/BIM
    ${Arch_presets}
)

IF (BUILD_GUI)
    fc_target_copy_resource(BIM
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/BIM
        Arch_rc.py)
ENDIF (BUILD_GUI)

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
        ${nativeifc_SRCS}
    DESTINATION Mod/BIM/nativeifc
)

INSTALL(
    DIRECTORY
        Presets
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/BIM
)

INSTALL(
    FILES
        ${BIMGuiIcon_SVG}
    DESTINATION
       "${CMAKE_INSTALL_DATADIR}/Mod/BIM/Resources/icons"
)

