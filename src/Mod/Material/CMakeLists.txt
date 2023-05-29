if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Material_QRC_SRCS Resources/Material.qrc)
ENDIF (BUILD_GUI)

SET(Material_SRCS
    Init.py
    InitGui.py
    Material.py
    importFCMat.py
    MaterialEditor.py
    Materials.py
    Resources/ui/materials.ui
    materials-editor.ui
    Templatematerial.yml
)
SOURCE_GROUP("Module" FILES ${Material_SRCS})

SET (MaterialTools_Files
    materialtools/__init__.py
    materialtools/cardutils.py
)
SET (Material_Icon_Files
    Resources/icons/preview-rendered.svg
    Resources/icons/preview-vector.svg
)

# collect all the material cards:
#FILE( GLOB MaterialLib_Files ./StandardMaterial/*.FCMat ./StandardMaterial/*.txt  )

SET (MaterialLib_Files
    Resources/Materials/StandardMaterial/None.FCMat
    Resources/Materials/StandardMaterial/ABS-Generic.FCMat
    Resources/Materials/StandardMaterial/Acrylic-Glass-Generic.FCMat
    Resources/Materials/StandardMaterial/AlMg3F24.FCMat
    Resources/Materials/StandardMaterial/AlMgSi1F31.FCMat
    Resources/Materials/StandardMaterial/Aluminum-6061-T6.FCMat
    Resources/Materials/StandardMaterial/Aluminum-Generic.FCMat
    Resources/Materials/StandardMaterial/AlZn4-5Mg1F35.FCMat
    Resources/Materials/StandardMaterial/CalculiX-Steel.FCMat
    Resources/Materials/StandardMaterial/Concrete-Generic.FCMat
    Resources/Materials/StandardMaterial/Copper-Generic.FCMat
    Resources/Materials/StandardMaterial/Glass-E-GlassFibre.FCMat
    Resources/Materials/StandardMaterial/Glass-Generic.FCMat
    Resources/Materials/StandardMaterial/Glass-S2-GlassFibre.FCMat
    Resources/Materials/StandardMaterial/Graphite.FCMat
    Resources/Materials/StandardMaterial/Iron-Generic.FCMat
    Resources/Materials/StandardMaterial/Invar-Generic.FCMat
    Resources/Materials/StandardMaterial/PA6-Generic.FCMat
    Resources/Materials/StandardMaterial/PET-Generic.FCMat
    Resources/Materials/StandardMaterial/PLA-Generic.FCMat
    Resources/Materials/StandardMaterial/PP-Generic.FCMat
    Resources/Materials/StandardMaterial/PTFE-Generic.FCMat
    Resources/Materials/StandardMaterial/PVC-Generic.FCMat
    Resources/Materials/StandardMaterial/Reinforcement-FIB-B500.FCMat
    Resources/Materials/StandardMaterial/Steel-15CrNi6.FCMat
    Resources/Materials/StandardMaterial/Steel-17CrNiMo6.FCMat
    Resources/Materials/StandardMaterial/Steel-1C22.FCMat
    Resources/Materials/StandardMaterial/Steel-1C35.FCMat
    Resources/Materials/StandardMaterial/Steel-1C45.FCMat
    Resources/Materials/StandardMaterial/Steel-1C60.FCMat
    Resources/Materials/StandardMaterial/Steel-20NiCrMo2.FCMat
    Resources/Materials/StandardMaterial/Steel-28Mn6.FCMat
    Resources/Materials/StandardMaterial/Steel-2C10.FCMat
    Resources/Materials/StandardMaterial/Steel-30CrNiMo8.FCMat
    Resources/Materials/StandardMaterial/Steel-34CrNiMo6.FCMat
    Resources/Materials/StandardMaterial/Steel-36CrNiMo4.FCMat
    Resources/Materials/StandardMaterial/Steel-36NiCrMo16.FCMat
    Resources/Materials/StandardMaterial/Steel-3C15.FCMat
    Resources/Materials/StandardMaterial/Steel-3C22.FCMat
    Resources/Materials/StandardMaterial/Steel-3C35.FCMat
    Resources/Materials/StandardMaterial/Steel-3V45.FCMat
    Resources/Materials/StandardMaterial/Steel-C10.FCMat
    Resources/Materials/StandardMaterial/Steel-C15.FCMat
    Resources/Materials/StandardMaterial/Steel-C22E.FCMat
    Resources/Materials/StandardMaterial/Steel-C25E.FCMat
    Resources/Materials/StandardMaterial/Steel-C30E.FCMat
    Resources/Materials/StandardMaterial/Steel-C40E.FCMat
    Resources/Materials/StandardMaterial/Steel-C50E.FCMat
    Resources/Materials/StandardMaterial/Steel-C55E.FCMat
    Resources/Materials/StandardMaterial/Steel-C60E.FCMat
    Resources/Materials/StandardMaterial/Steel-E295-GC.FCMat
    Resources/Materials/StandardMaterial/Steel-E295.FCMat
    Resources/Materials/StandardMaterial/Steel-E335-GC.FCMat
    Resources/Materials/StandardMaterial/Steel-E335.FCMat
    Resources/Materials/StandardMaterial/Steel-E360-GC.FCMat
    Resources/Materials/StandardMaterial/Steel-E360.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJL-100.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJL-150.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJL-200.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJL-250.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJL-300.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJL-350.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJMB-350-10.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJMB-550-4.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJMB-650-2.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJMW-350-4.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJMW-360-12.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJMW-400-5.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJMW-450-7.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJS-400-15.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJS-500-7.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJS-600-3.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJS-700-2.FCMat
    Resources/Materials/StandardMaterial/Steel-EN-GJS-800-1.FCMat
    Resources/Materials/StandardMaterial/Steel-G16Mn5.FCMat
    Resources/Materials/StandardMaterial/Steel-G200.FCMat
    Resources/Materials/StandardMaterial/Steel-G20Mn5.FCMat
    Resources/Materials/StandardMaterial/Steel-G230.FCMat
    Resources/Materials/StandardMaterial/Steel-G260.FCMat
    Resources/Materials/StandardMaterial/Steel-G300.FCMat
    Resources/Materials/StandardMaterial/Steel-G30Mn5.FCMat
    Resources/Materials/StandardMaterial/Steel-Generic.FCMat
    Resources/Materials/StandardMaterial/Steel-S185.FCMat
    Resources/Materials/StandardMaterial/Steel-S235JO.FCMat
    Resources/Materials/StandardMaterial/Steel-S235JR.FCMat
    Resources/Materials/StandardMaterial/Steel-S235JRG1.FCMat
    Resources/Materials/StandardMaterial/Steel-S260NC.FCMat
    Resources/Materials/StandardMaterial/Steel-S275JO.FCMat
    Resources/Materials/StandardMaterial/Steel-S275JR.FCMat
    Resources/Materials/StandardMaterial/Steel-S275N.FCMat
    Resources/Materials/StandardMaterial/Steel-S335JO.FCMat
    Resources/Materials/StandardMaterial/Steel-S335JR.FCMat
    Resources/Materials/StandardMaterial/Steel-S335N.FCMat
    Resources/Materials/StandardMaterial/Steel-S340MC.FCMat
    Resources/Materials/StandardMaterial/Steel-S355J2G3.FCMat
    Resources/Materials/StandardMaterial/Steel-S380MC.FCMat
    Resources/Materials/StandardMaterial/Steel-S420MC.FCMat
    Resources/Materials/StandardMaterial/Steel-S420N.FCMat
    Resources/Materials/StandardMaterial/Steel-S460MC.FCMat
    Resources/Materials/StandardMaterial/Steel-S460N.FCMat
    Resources/Materials/StandardMaterial/Steel-S500MC.FCMat
    Resources/Materials/StandardMaterial/Steel-S550MC.FCMat
    Resources/Materials/StandardMaterial/Steel-S690MC.FCMat
    Resources/Materials/StandardMaterial/Steel-St-37-2K.FCMat
    Resources/Materials/StandardMaterial/Steel-St-E-255.FCMat
    Resources/Materials/StandardMaterial/Steel-St-E-315.FCMat
    Resources/Materials/StandardMaterial/Steel-St-E-380.FCMat
    Resources/Materials/StandardMaterial/Steel-St-E-460.FCMat
    Resources/Materials/StandardMaterial/Steel-St-E-500.FCMat
    Resources/Materials/StandardMaterial/Steel-X2CrNiMoN17-13-3.FCMat
    Resources/Materials/StandardMaterial/Steel-X2CrNiN24-4.FCMat
    Resources/Materials/StandardMaterial/Steel-X39CrMo17-1.FCMat
    Resources/Materials/StandardMaterial/Steel-X3CrNiMo13-14.FCMat
    Resources/Materials/StandardMaterial/Steel-X5CrNi18-10.FCMat
    Resources/Materials/StandardMaterial/Steel-X5CrNiMo17-12-2.FCMat
    Resources/Materials/StandardMaterial/Steel-X6CrNiTi18-10.FCMat
    Resources/Materials/StandardMaterial/TEMPLATE.FCMat
    Resources/Materials/StandardMaterial/Ti-6Al-4V.FCMat
    Resources/Materials/StandardMaterial/Wood-Generic.FCMat
    Resources/Materials/StandardMaterial/Readme.txt
)
SOURCE_GROUP("MatLib" FILES ${MaterialLib_Files})

SET (FluidMaterial_Files
    Resources/Materials/FluidMaterial/None.FCMat
    Resources/Materials/FluidMaterial/Air.FCMat
    Resources/Materials/FluidMaterial/Argon.FCMat
    Resources/Materials/FluidMaterial/Carbon_dioxide.FCMat
    Resources/Materials/FluidMaterial/Nitrogen.FCMat
    Resources/Materials/FluidMaterial/Water.FCMat
    Resources/Materials/FluidMaterial/Readme.md
)
SOURCE_GROUP("MatLib" FILES ${FluidMaterial_Files})

ADD_CUSTOM_TARGET(Material ALL
    SOURCES ${Material_SRCS} ${Material_QRC_SRCS}
)

fc_target_copy_resource(Material
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Material
    ${Material_SRCS})


ADD_CUSTOM_TARGET(MaterialToolsLib ALL
    SOURCES ${MaterialTools_Files}
)

ADD_CUSTOM_TARGET(MaterialIconsLib ALL
    SOURCES ${Material_Icon_Files}
)

fc_target_copy_resource(MaterialToolsLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Material
    ${MaterialTools_Files})

fc_target_copy_resource(MaterialIconsLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Material
    ${Material_Icon_Files})

INSTALL(FILES ${MaterialTools_Files} DESTINATION Mod/Material/materialtools)
INSTALL(FILES ${Material_Icon_Files} DESTINATION Mod/Material/Resources/icons)

IF (BUILD_GUI)
    fc_target_copy_resource(Material
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Material
        Material_rc.py)
ENDIF (BUILD_GUI)

ADD_CUSTOM_TARGET(MaterialLib ALL
    SOURCES ${MaterialLib_Files}
)
ADD_CUSTOM_TARGET(FluidMaterialLib ALL
    SOURCES ${FluidMaterial_Files}
)

# When a target copies files to different output directories then apparently it always builds the project
# which is very annoying. So, the trick is to split this into two targets to avoid this behaviour.
fc_target_copy_resource(MaterialLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material/
    ${MaterialLib_Files})
fc_target_copy_resource(FluidMaterialLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material/
    ${FluidMaterial_Files})

INSTALL(
    FILES ${Material_SRCS} ${Material_QRC_SRCS}
    DESTINATION Mod/Material
)

INSTALL(
    DIRECTORY
        StandardMaterial
    DIRECTORY
        FluidMaterial
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Material
    FILES_MATCHING PATTERN "*.FCMat*"
)
