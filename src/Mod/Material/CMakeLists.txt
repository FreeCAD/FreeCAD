add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Material_QRC_SRCS Resources/Material.qrc)
ENDIF (BUILD_GUI)

SET(MaterialScripts_Files
    Init.py
    InitGui.py
    Material.py
    importFCMat.py
    MaterialEditor.py
    Materials.py
    ModelSelect.py
    ValueEditor.py
    Resources/ui/materials.ui
    Resources/ui/materials-editor.ui
    Resources/ui/ValueEditor.ui
    Resources/ui/ModelSelect.ui
    Templatematerial.yml
)
# SOURCE_GROUP("MaterialScripts" FILES ${MaterialScripts_Files})

SET (MaterialTools_Files
    materialtools/__init__.py
    materialtools/cardutils.py
    materialtools/MaterialModels.py
)
SET (Material_Icon_Files
    Resources/icons/preview-rendered.svg
    Resources/icons/preview-vector.svg
)

# collect all the material cards:
#FILE( GLOB MaterialLib_Files ./StandardMaterial/*.FCMat ./StandardMaterial/*.txt  )

SET (MaterialLib_Files
    Resources/Materials/StandardMaterial/None.FCMat
    Resources/Materials/StandardMaterial/TEMPLATE.FCMat
    Resources/Materials/StandardMaterial/Readme.txt
    Resources/Materials/StandardMaterial/Aggregate/Concrete-EN-C35_45.FCMat
    Resources/Materials/StandardMaterial/Aggregate/Concrete-Generic.FCMat
    Resources/Materials/StandardMaterial/Aggregate/Reinforcement-FIB-B500.FCMat
    Resources/Materials/StandardMaterial/Carbon/Graphite.FCMat
    Resources/Materials/StandardMaterial/Glass/Glass-E-GlassFibre.FCMat
    Resources/Materials/StandardMaterial/Glass/Glass-Generic.FCMat
    Resources/Materials/StandardMaterial/Glass/Glass-S2-GlassFibre.FCMat
    Resources/Materials/StandardMaterial/Metal/Alloys/Invar-Generic.FCMat
    Resources/Materials/StandardMaterial/Metal/Aluminum/AlMg3F24.FCMat
    Resources/Materials/StandardMaterial/Metal/Aluminum/AlMgSi1F31.FCMat
    Resources/Materials/StandardMaterial/Metal/Aluminum/Aluminum-6061-T6.FCMat
    Resources/Materials/StandardMaterial/Metal/Aluminum/Aluminum-Generic.FCMat
    Resources/Materials/StandardMaterial/Metal/Aluminum/AlZn4-5Mg1F35.FCMat
    Resources/Materials/StandardMaterial/Metal/Copper/Copper-Generic.FCMat
    Resources/Materials/StandardMaterial/Metal/Iron/Iron-Generic.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/CalculiX-Steel.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-15CrNi6.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-17CrNiMo6.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-1C22.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-1C35.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-1C45.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-1C60.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-20NiCrMo2.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-28Mn6.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-2C10.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-30CrNiMo8.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-34CrNiMo6.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-36CrNiMo4.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-36NiCrMo16.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-3C15.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-3C22.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-3C35.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-3V45.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-C10.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-C15.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-C22E.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-C25E.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-C30E.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-C40E.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-C50E.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-C55E.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-C60E.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-E295-GC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-E295.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-E335-GC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-E335.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-E360-GC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-E360.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJL-100.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJL-150.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJL-200.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJL-250.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJL-300.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJL-350.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJMB-350-10.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJMB-550-4.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJMB-650-2.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJMW-350-4.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJMW-360-12.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJMW-400-5.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJMW-450-7.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJS-400-15.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJS-500-7.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJS-600-3.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJS-700-2.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-EN-GJS-800-1.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-G16Mn5.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-G200.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-G20Mn5.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-G230.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-G260.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-G300.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-G30Mn5.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-Generic.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S185.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S235JO.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S235JR.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S235JRG1.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S260NC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S275JO.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S275JR.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S275N.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S335JO.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S335JR.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S335N.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S340MC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S355J2G3.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S380MC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S420MC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S420N.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S460MC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S460N.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S500MC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S550MC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-S690MC.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-St-37-2K.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-St-E-255.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-St-E-315.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-St-E-380.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-St-E-460.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-St-E-500.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-X2CrNiMoN17-13-3.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-X2CrNiN24-4.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-X39CrMo17-1.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-X3CrNiMo13-14.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-X5CrNi18-10.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-X5CrNiMo17-12-2.FCMat
    Resources/Materials/StandardMaterial/Metal/Steel/Steel-X6CrNiTi18-10.FCMat
    Resources/Materials/StandardMaterial/Metal/Titanium/Ti-6Al-4V.FCMat
    Resources/Materials/StandardMaterial/Thermoplast/ABS-Generic.FCMat
    Resources/Materials/StandardMaterial/Thermoplast/Acrylic-Glass-Generic.FCMat
    Resources/Materials/StandardMaterial/Thermoplast/PA6-Generic.FCMat
    Resources/Materials/StandardMaterial/Thermoplast/PET-Generic.FCMat
    Resources/Materials/StandardMaterial/Thermoplast/PLA-Generic.FCMat
    Resources/Materials/StandardMaterial/Thermoplast/PP-Generic.FCMat
    Resources/Materials/StandardMaterial/Thermoplast/PTFE-Generic.FCMat
    Resources/Materials/StandardMaterial/Thermoplast/PVC-Generic.FCMat
    Resources/Materials/StandardMaterial/Wood/Wood-Generic.FCMat
)
# SOURCE_GROUP("MatLib" FILES ${MaterialLib_Files})

SET (FluidMaterial_Files
    Resources/Materials/FluidMaterial/None.FCMat
    Resources/Materials/FluidMaterial/Air.FCMat
    Resources/Materials/FluidMaterial/Argon.FCMat
    Resources/Materials/FluidMaterial/Carbon_dioxide.FCMat
    Resources/Materials/FluidMaterial/Nitrogen.FCMat
    Resources/Materials/FluidMaterial/Water.FCMat
    Resources/Materials/FluidMaterial/Readme.md
)
# SOURCE_GROUP("MatLib" FILES ${FluidMaterial_Files})

SET (MaterialModel_Files
    Resources/Models/Architectural/default.yml
    Resources/Models/Costs/default.yml
    Resources/Models/Electromagnetic/default.yml
    Resources/Models/Fluid/default.yml
    Resources/Models/Mechanical/Density.yml
    Resources/Models/Mechanical/HypotheticalExample.yml
    Resources/Models/Mechanical/IsotropicLinearElastic.yml
    Resources/Models/Mechanical/LinearElastic.yml
    Resources/Models/Mechanical/OgdenYld2004p18.yml
    Resources/Models/Mechanical/OrthotropicLinearElastic.yml
    Resources/Models/Rendering/Rendering.yml
    Resources/Models/Rendering/VectorRendering.yml
    Resources/Models/Thermal/default.yml
)

ADD_CUSTOM_TARGET(MaterialScripts ALL
    SOURCES ${MaterialScripts_Files} ${Material_QRC_SRCS}
)

fc_target_copy_resource(MaterialScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Material
    ${MaterialScripts_Files})

INSTALL(FILES ${MaterialScripts_Files} DESTINATION Mod/Material)


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

# IF (BUILD_GUI)
#     fc_target_copy_resource(Material
#         ${CMAKE_CURRENT_BINARY_DIR}
#         ${CMAKE_BINARY_DIR}/Mod/Material
#         Material_rc.py)
# ENDIF (BUILD_GUI)

ADD_CUSTOM_TARGET(MaterialLib ALL
    SOURCES ${MaterialLib_Files}
)
ADD_CUSTOM_TARGET(FluidMaterialLib ALL
    SOURCES ${FluidMaterial_Files}
)
ADD_CUSTOM_TARGET(MaterialModelLib ALL
    SOURCES ${MaterialModel_Files}
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
fc_target_copy_resource(MaterialModelLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material/
    ${MaterialModel_Files})

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

# ADD_CUSTOM_TARGET(MaterialScriptsTarget ALL
#     SOURCES ${MaterialScripts}
# )
# fc_copy_sources(MaterialScriptsTarget "${CMAKE_BINARY_DIR}/Mod/Material" ${MaterialScripts})

# INSTALL(
#     FILES
#         ${MaterialScripts}
#     DESTINATION
#         ${CMAKE_INSTALL_DATADIR}/Mod/Material
# )
