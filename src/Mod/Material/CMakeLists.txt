IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Material_QRC_SRCS Resources/Material.qrc)
ENDIF (BUILD_GUI)

SET(Material_SRCS
    Init.py
    InitGui.py
    Material.py
    importFCMat.py
    MaterialEditor.py
    materials-editor.ui
    Templatematerial.yml
)
SOURCE_GROUP("Module" FILES ${Material_SRCS})

SET (MaterialTools_Files
    materialtools/__init__.py
    materialtools/cardutils.py
)

# collect all the material cards:
#FILE( GLOB MaterialLib_Files ./StandardMaterial/*.FCMat ./StandardMaterial/*.txt  )

SET (MaterialLib_Files
    StandardMaterial/None.FCMat
    StandardMaterial/ABS-Generic.FCMat
    StandardMaterial/Acrylic-Glass-Generic.FCMat
    StandardMaterial/AlMg3F24.FCMat
    StandardMaterial/AlMgSi1F31.FCMat
    StandardMaterial/Aluminum-6061-T6.FCMat
    StandardMaterial/Aluminum-Generic.FCMat
    StandardMaterial/AlZn4-5Mg1F35.FCMat
    StandardMaterial/CalculiX-Steel.FCMat
    StandardMaterial/Concrete-Generic.FCMat
    StandardMaterial/Copper-Generic.FCMat
    StandardMaterial/Glass-E-GlassFibre.FCMat
    StandardMaterial/Glass-Generic.FCMat
    StandardMaterial/Glass-S2-GlassFibre.FCMat
    StandardMaterial/Graphite.FCMat
    StandardMaterial/Invar-Generic.FCMat
    StandardMaterial/PA6-Generic.FCMat
    StandardMaterial/PET-Generic.FCMat
    StandardMaterial/PLA-Generic.FCMat
    StandardMaterial/PP-Generic.FCMat
    StandardMaterial/PTFE-Generic.FCMat
    StandardMaterial/PVC-Generic.FCMat
    StandardMaterial/Reinforcement-FIB-B500.FCMat
    StandardMaterial/Steel-15CrNi6.FCMat
    StandardMaterial/Steel-17CrNiMo6.FCMat
    StandardMaterial/Steel-1C22.FCMat
    StandardMaterial/Steel-1C35.FCMat
    StandardMaterial/Steel-1C45.FCMat
    StandardMaterial/Steel-1C60.FCMat
    StandardMaterial/Steel-20NiCrMo2.FCMat
    StandardMaterial/Steel-28Mn6.FCMat
    StandardMaterial/Steel-2C10.FCMat
    StandardMaterial/Steel-30CrNiMo8.FCMat
    StandardMaterial/Steel-34CrNiMo6.FCMat
    StandardMaterial/Steel-36CrNiMo4.FCMat
    StandardMaterial/Steel-36NiCrMo16.FCMat
    StandardMaterial/Steel-3C15.FCMat
    StandardMaterial/Steel-3C22.FCMat
    StandardMaterial/Steel-3C35.FCMat
    StandardMaterial/Steel-3V45.FCMat
    StandardMaterial/Steel-C10.FCMat
    StandardMaterial/Steel-C15.FCMat
    StandardMaterial/Steel-C22E.FCMat
    StandardMaterial/Steel-C25E.FCMat
    StandardMaterial/Steel-C30E.FCMat
    StandardMaterial/Steel-C40E.FCMat
    StandardMaterial/Steel-C50E.FCMat
    StandardMaterial/Steel-C55E.FCMat
    StandardMaterial/Steel-C60E.FCMat
    StandardMaterial/Steel-E295-GC.FCMat
    StandardMaterial/Steel-E295.FCMat
    StandardMaterial/Steel-E335-GC.FCMat
    StandardMaterial/Steel-E335.FCMat
    StandardMaterial/Steel-E360-GC.FCMat
    StandardMaterial/Steel-E360.FCMat
    StandardMaterial/Steel-EN-GJL-100.FCMat
    StandardMaterial/Steel-EN-GJL-150.FCMat
    StandardMaterial/Steel-EN-GJL-200.FCMat
    StandardMaterial/Steel-EN-GJL-250.FCMat
    StandardMaterial/Steel-EN-GJL-300.FCMat
    StandardMaterial/Steel-EN-GJL-350.FCMat
    StandardMaterial/Steel-EN-GJMB-350-10.FCMat
    StandardMaterial/Steel-EN-GJMB-550-4.FCMat
    StandardMaterial/Steel-EN-GJMB-650-2.FCMat
    StandardMaterial/Steel-EN-GJMW-350-4.FCMat
    StandardMaterial/Steel-EN-GJMW-360-12.FCMat
    StandardMaterial/Steel-EN-GJMW-400-5.FCMat
    StandardMaterial/Steel-EN-GJMW-450-7.FCMat
    StandardMaterial/Steel-EN-GJS-400-15.FCMat
    StandardMaterial/Steel-EN-GJS-500-7.FCMat
    StandardMaterial/Steel-EN-GJS-600-3.FCMat
    StandardMaterial/Steel-EN-GJS-700-2.FCMat
    StandardMaterial/Steel-EN-GJS-800-1.FCMat
    StandardMaterial/Steel-G16Mn5.FCMat
    StandardMaterial/Steel-G200.FCMat
    StandardMaterial/Steel-G20Mn5.FCMat
    StandardMaterial/Steel-G230.FCMat
    StandardMaterial/Steel-G260.FCMat
    StandardMaterial/Steel-G300.FCMat
    StandardMaterial/Steel-G30Mn5.FCMat
    StandardMaterial/Steel-Generic.FCMat
    StandardMaterial/Steel-S185.FCMat
    StandardMaterial/Steel-S235JO.FCMat
    StandardMaterial/Steel-S235JR.FCMat
    StandardMaterial/Steel-S235JRG1.FCMat
    StandardMaterial/Steel-S260NC.FCMat
    StandardMaterial/Steel-S275JO.FCMat
    StandardMaterial/Steel-S275JR.FCMat
    StandardMaterial/Steel-S275N.FCMat
    StandardMaterial/Steel-S335JO.FCMat
    StandardMaterial/Steel-S335JR.FCMat
    StandardMaterial/Steel-S335N.FCMat
    StandardMaterial/Steel-S340MC.FCMat
    StandardMaterial/Steel-S355J2G3.FCMat
    StandardMaterial/Steel-S380MC.FCMat
    StandardMaterial/Steel-S420MC.FCMat
    StandardMaterial/Steel-S420N.FCMat
    StandardMaterial/Steel-S460MC.FCMat
    StandardMaterial/Steel-S460N.FCMat
    StandardMaterial/Steel-S500MC.FCMat
    StandardMaterial/Steel-S550MC.FCMat
    StandardMaterial/Steel-S690MC.FCMat
    StandardMaterial/Steel-St-37-2K.FCMat
    StandardMaterial/Steel-St-E-255.FCMat
    StandardMaterial/Steel-St-E-315.FCMat
    StandardMaterial/Steel-St-E-380.FCMat
    StandardMaterial/Steel-St-E-460.FCMat
    StandardMaterial/Steel-St-E-500.FCMat
    StandardMaterial/Steel-X2CrNiMoN17-13-3.FCMat
    StandardMaterial/Steel-X2CrNiN24-4.FCMat
    StandardMaterial/Steel-X39CrMo17-1.FCMat
    StandardMaterial/Steel-X3CrNiMo13-14.FCMat
    StandardMaterial/Steel-X5CrNi18-10.FCMat
    StandardMaterial/Steel-X5CrNiMo17-12-2.FCMat
    StandardMaterial/Steel-X6CrNiTi18-10.FCMat
    StandardMaterial/TEMPLATE.FCMat
    StandardMaterial/Wood-Generic.FCMat
    StandardMaterial/Readme.txt
)
SOURCE_GROUP("MatLib" FILES ${MaterialLib_Files})

SET (FluidMaterial_Files
    FluidMaterial/None.FCMat
    FluidMaterial/Air.FCMat
    FluidMaterial/Argon.FCMat
    FluidMaterial/Nitrogen.FCMat
    FluidMaterial/Water.FCMat
    FluidMaterial/Readme.md
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

fc_target_copy_resource(MaterialToolsLib 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Material
    ${MaterialTools_Files})

INSTALL(FILES ${MaterialTools_Files} DESTINATION Mod/Material/materialtools)


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
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material
    ${MaterialLib_Files})
fc_target_copy_resource(FluidMaterialLib 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material
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
