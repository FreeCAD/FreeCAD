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
    importIFC.py
    importIFClegacy.py
    importIFCHelper.py
    exportIFCHelper.py
    Arch.py
    ArchBuilding.py
    ArchFloor.py
    ArchSite.py
    ArchStructure.py
    ArchCommands.py
    ArchSectionPlane.py
    importDAE.py
    importOBJ.py
    ArchWindow.py
    ArchAxis.py
    ArchVRM.py
    ArchRoof.py
    ArchStairs.py
    importWebGL.py
    importJSON.py
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
    import3DS.py
    ArchPrecast.py
    importSH3D.py
    ArchPipe.py
    ArchNesting.py
    ArchBuildingPart.py
    ArchReference.py
    ArchFence.py
    OfflineRenderingUtils.py
    exportIFC.py
    ArchTruss.py
    ArchCurtainWall.py
    importSHP.py
)

SET(Arch_GUI_tools
    archguitools/__init__.py
    archguitools/gui_wall.py
    archguitools/gui_joinwalls.py
)

SET(Arch_make
    archmake/__init__.py
    archmake/make_wall.py
    archmake/joinwalls.py
)

SET(Arch_objects
    archobjects/__init__.py
    archobjects/wall.py
    archobjects/wallsegment.py
)

SET(Arch_view_providers
    archviewproviders/__init__.py
    archviewproviders/view_wall.py
    archviewproviders/view_wallsegment.py
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
)

SOURCE_GROUP("" FILES ${Arch_SRCS})

SET(ArchGuiIcon_SVG
    Resources/icons/ArchWorkbench.svg
)

ADD_CUSTOM_TARGET(Arch ALL
    SOURCES ${Arch_SRCS} ${Arch_QRC_SRCS} ${Dice3DS_SRCS} ${Arch_presets} ${ArchGuiIcon_SVG} ${Arch_GUI_tools} ${Arch_make} ${Arch_objects} ${Arch_view_providers}
)

fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Arch_SRCS})
fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Dice3DS_SRCS})
fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Arch_GUI_tools})
fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Arch_make})
fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Arch_objects})
fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Arch_view_providers})
fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Arch" ${ArchGuiIcon_SVG})

fc_target_copy_resource(Arch
    ${CMAKE_SOURCE_DIR}/src/Mod/Arch
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Arch
    ${Arch_presets}
)

IF (BUILD_GUI)
    fc_target_copy_resource(Arch
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Arch
        Arch_rc.py)
ENDIF (BUILD_GUI)

INSTALL(
    FILES
        ${Arch_SRCS}
        ${Arch_QRC_SRCS}
    DESTINATION Mod/Arch
)

INSTALL(
    FILES
        ${Arch_GUI_tools}
    DESTINATION Mod/Arch/archguitools
)

INSTALL(
    FILES
        ${Arch_make}
    DESTINATION Mod/Arch/archmake
)

INSTALL(
    FILES
        ${Arch_objects}
    DESTINATION Mod/Arch/archobjects
)

INSTALL(
    FILES
        ${Arch_view_providers}
    DESTINATION Mod/Arch/archviewproviders
)

INSTALL(
    FILES
        ${Dice3DS_SRCS}
    DESTINATION Mod/Arch/Dice3DS
)
INSTALL(
    DIRECTORY
        Presets
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Arch
)
INSTALL(
    FILES
        ${ArchGuiIcon_SVG}
    DESTINATION
       "${CMAKE_INSTALL_DATADIR}/Mod/Arch/Resources/icons"
)

