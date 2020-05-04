IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Arch_QRC_SRCS Resources/Arch.qrc)
ENDIF (BUILD_GUI)

SET(Arch_SRCS
    Init.py
    InitGui.py
    archadd.py
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

SET(Arch_make_functions
    archmake/__init__.py
    archmake/make_base_rebar.py
    archmake/make_reinforcement_custom.py
    archmake/make_reinforcement_generic.py
    archmake/make_reinforcement_individual.py
    archmake/make_reinforcement_lattice.py
    archmake/make_reinforcement_linear.py
)

SET(Arch_objects
    archobjects/__init__.py
    archobjects/base_rebar.py
    archobjects/reinforcement_custom.py
    archobjects/reinforcement_generic.py
    archobjects/reinforcement_individual.py
    archobjects/reinforcement_lattice.py
    archobjects/reinforcement_linear.py
)

SET(Arch_view_providers
    archviewproviders/__init__.py
    archviewproviders/view_base_rebar.py
    archviewproviders/view_rebar_generic.py
    archviewproviders/view_reinforcement_custom.py
    archviewproviders/view_reinforcement_generic.py
    archviewproviders/view_reinforcement_individual.py
    archviewproviders/view_reinforcement_lattice.py
    archviewproviders/view_reinforcement_linear.py
)

SET(Arch_SRCS_all
    ${Arch_SRCS}
    ${Dice3DS_SRCS}
    ${Arch_presets}
    ${Arch_make_functions}
    ${Arch_objects}
    ${Arch_view_providers}
)

SOURCE_GROUP("" FILES ${Arch_SRCS_all})

SET(ArchGuiIcon_SVG
    Resources/icons/ArchWorkbench.svg
)

ADD_CUSTOM_TARGET(Arch ALL
    SOURCES ${Arch_SRCS_all} ${Arch_QRC_SRCS} ${ArchGuiIcon_SVG}
)

fc_copy_sources(Arch "${CMAKE_BINARY_DIR}/Mod/Arch" ${Arch_SRCS_all})
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

INSTALL(FILES ${Dice3DS_SRCS} DESTINATION Mod/Arch/Dice3DS)
INSTALL(FILES ${Arch_make_functions} DESTINATION Mod/Arch/archmake)
INSTALL(FILES ${Arch_objects} DESTINATION Mod/Arch/archobjects)
INSTALL(FILES ${Arch_view_providers} DESTINATION Mod/Arch/archviewproviders)

INSTALL(DIRECTORY Presets DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Arch)

INSTALL(
    FILES
        ${ArchGuiIcon_SVG}
    DESTINATION
       "${CMAKE_INSTALL_DATADIR}/Mod/Arch/Resources/icons"
)

