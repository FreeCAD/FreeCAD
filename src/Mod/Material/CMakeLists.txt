add_subdirectory(App)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

IF(BUILD_GUI)
    PYSIDE_WRAP_RC(Material_QRC_SRCS Resources/Material.qrc)
ENDIF(BUILD_GUI)

SET(MaterialScripts_Files
    Init.py
    InitGui.py
    importFCMat.py
    MaterialEditor.py
    TestMaterialsApp.py
    Resources/ui/materials-editor.ui
    Templatematerial.yml
)

# SOURCE_GROUP("MaterialScripts" FILES ${MaterialScripts_Files})
SET(MaterialTools_Files
    materialtools/__init__.py
    materialtools/cardutils.py
    materialtools/MaterialModels.py
)
SET(Material_Icon_Files
    Resources/icons/preview-rendered.svg
    Resources/icons/preview-vector.svg
)

# collect all the material cards:
SET(MaterialLib_Files
    Resources/Materials/Standard/Aggregate/Concrete-EN-C35_45.FCMat
    Resources/Materials/Standard/Aggregate/Concrete-Generic.FCMat
    Resources/Materials/Standard/Aggregate/Reinforcement-FIB-B500.FCMat
    Resources/Materials/Standard/Carbon/Graphite.FCMat
    Resources/Materials/Standard/Glass/Glass-E-GlassFibre.FCMat
    Resources/Materials/Standard/Glass/Glass-Generic.FCMat
    Resources/Materials/Standard/Glass/Glass-S2-GlassFibre.FCMat
    Resources/Materials/Standard/Metal/Alloys/Invar-Generic.FCMat
    Resources/Materials/Standard/Metal/Aluminum/AlMg3F24.FCMat
    Resources/Materials/Standard/Metal/Aluminum/AlMgSi1F31.FCMat
    Resources/Materials/Standard/Metal/Aluminum/Aluminum-6061-T6.FCMat
    Resources/Materials/Standard/Metal/Aluminum/Aluminum-Generic.FCMat
    Resources/Materials/Standard/Metal/Aluminum/AlZn4-5Mg1F35.FCMat
    Resources/Materials/Standard/Metal/Copper/Copper-Generic.FCMat
    Resources/Materials/Standard/Metal/Iron/Iron-Generic.FCMat
    Resources/Materials/Standard/Metal/Steel/CalculiX-Steel.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-15CrNi6.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-17CrNiMo6.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-1C22.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-1C35.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-1C45.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-1C60.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-20NiCrMo2.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-28Mn6.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-2C10.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-30CrNiMo8.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-34CrNiMo6.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-36CrNiMo4.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-36NiCrMo16.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-3C15.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-3C22.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-3C35.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-3V45.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-C10.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-C15.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-C22E.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-C25E.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-C30E.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-C40E.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-C50E.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-C55E.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-C60E.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-E295-GC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-E295.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-E335-GC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-E335.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-E360-GC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-E360.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJL-100.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJL-150.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJL-200.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJL-250.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJL-300.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJL-350.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJMB-350-10.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJMB-550-4.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJMB-650-2.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJMW-350-4.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJMW-360-12.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJMW-400-5.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJMW-450-7.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJS-400-15.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJS-500-7.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJS-600-3.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJS-700-2.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-EN-GJS-800-1.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-G16Mn5.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-G200.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-G20Mn5.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-G230.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-G260.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-G300.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-G30Mn5.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-Generic.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S185.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S235JO.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S235JR.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S235JRG1.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S260NC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S275JO.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S275JR.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S275N.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S335JO.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S335JR.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S335N.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S340MC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S355J2G3.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S380MC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S420MC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S420N.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S460MC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S460N.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S500MC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S550MC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-S690MC.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-St-37-2K.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-St-E-255.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-St-E-315.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-St-E-380.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-St-E-460.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-St-E-500.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-X2CrNiMoN17-13-3.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-X2CrNiN24-4.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-X39CrMo17-1.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-X3CrNiMo13-14.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-X5CrNi18-10.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-X5CrNiMo17-12-2.FCMat
    Resources/Materials/Standard/Metal/Steel/Steel-X6CrNiTi18-10.FCMat
    Resources/Materials/Standard/Metal/Titanium/Ti-6Al-4V.FCMat
    Resources/Materials/Standard/Thermoplast/ABS-Generic.FCMat
    Resources/Materials/Standard/Thermoplast/Acrylic-Glass-Generic.FCMat
    Resources/Materials/Standard/Thermoplast/PA6-Generic.FCMat
    Resources/Materials/Standard/Thermoplast/PET-Generic.FCMat
    Resources/Materials/Standard/Thermoplast/PLA-Generic.FCMat
    Resources/Materials/Standard/Thermoplast/PP-Generic.FCMat
    Resources/Materials/Standard/Thermoplast/PTFE-Generic.FCMat
    Resources/Materials/Standard/Thermoplast/PVC-Generic.FCMat
    Resources/Materials/Standard/Wood/Wood-Generic.FCMat
)

SET(FluidMaterial_Files
    Resources/Materials/Fluid/None.FCMat
    Resources/Materials/Fluid/Air.FCMat
    Resources/Materials/Fluid/Argon.FCMat
    "Resources/Materials/Fluid/Carbon Dioxide.FCMat"
    Resources/Materials/Fluid/Nitrogen.FCMat
    Resources/Materials/Fluid/Water.FCMat
)

SET(AppearanceLib_Files
    Resources/Materials/Appearance/Aluminum.FCMat
    Resources/Materials/Appearance/Brass.FCMat
    Resources/Materials/Appearance/Bronze.FCMat
    Resources/Materials/Appearance/Chrome.FCMat
    Resources/Materials/Appearance/Copper.FCMat
    Resources/Materials/Appearance/Default.FCMat
    Resources/Materials/Appearance/Emerald.FCMat
    Resources/Materials/Appearance/Gold.FCMat
    Resources/Materials/Appearance/Jade.FCMat
    Resources/Materials/Appearance/Metalized.FCMat
    "Resources/Materials/Appearance/Neon GNC.FCMat"
    "Resources/Materials/Appearance/Neon PHC.FCMat"
    Resources/Materials/Appearance/Obsidian.FCMat
    Resources/Materials/Appearance/Pewter.FCMat
    Resources/Materials/Appearance/Plaster.FCMat
    Resources/Materials/Appearance/Plastic.FCMat
    Resources/Materials/Appearance/Ruby.FCMat
    Resources/Materials/Appearance/Satin.FCMat
    "Resources/Materials/Appearance/Shiny Plastic.FCMat"
    Resources/Materials/Appearance/Silver.FCMat
    Resources/Materials/Appearance/Steel.FCMat
    Resources/Materials/Appearance/Stone.FCMat
)

SET(PatternLib_Files
    Resources/Materials/Patterns/PAT/Diagonal4.FCMat
    Resources/Materials/Patterns/PAT/Diagonal5.FCMat
    Resources/Materials/Patterns/PAT/Diamond.FCMat
    Resources/Materials/Patterns/PAT/Diamond2.FCMat
    Resources/Materials/Patterns/PAT/Diamond4.FCMat
    Resources/Materials/Patterns/PAT/Horizontal5.FCMat
    Resources/Materials/Patterns/PAT/Square.FCMat
    Resources/Materials/Patterns/PAT/Vertical5.FCMat
    "Resources/Materials/Patterns/Pattern Files/aluminum.FCMat"
    "Resources/Materials/Patterns/Pattern Files/brick01.FCMat"
    "Resources/Materials/Patterns/Pattern Files/concrete.FCMat"
    "Resources/Materials/Patterns/Pattern Files/cross.FCMat"
    "Resources/Materials/Patterns/Pattern Files/cuprous.FCMat"
    "Resources/Materials/Patterns/Pattern Files/diagonal1.FCMat"
    "Resources/Materials/Patterns/Pattern Files/diagonal2.FCMat"
    "Resources/Materials/Patterns/Pattern Files/earth.FCMat"
    "Resources/Materials/Patterns/Pattern Files/general_steel.FCMat"
    "Resources/Materials/Patterns/Pattern Files/glass.FCMat"
    "Resources/Materials/Patterns/Pattern Files/hatch45L.FCMat"
    "Resources/Materials/Patterns/Pattern Files/hatch45R.FCMat"
    "Resources/Materials/Patterns/Pattern Files/hbone.FCMat"
    "Resources/Materials/Patterns/Pattern Files/line.FCMat"
    "Resources/Materials/Patterns/Pattern Files/plastic.FCMat"
    "Resources/Materials/Patterns/Pattern Files/plus.FCMat"
    "Resources/Materials/Patterns/Pattern Files/simple.FCMat"
    "Resources/Materials/Patterns/Pattern Files/solid.FCMat"
    "Resources/Materials/Patterns/Pattern Files/square.FCMat"
    "Resources/Materials/Patterns/Pattern Files/steel.FCMat"
    "Resources/Materials/Patterns/Pattern Files/titanium.FCMat"
    "Resources/Materials/Patterns/Pattern Files/wood.FCMat"
    "Resources/Materials/Patterns/Pattern Files/woodgrain.FCMat"
    "Resources/Materials/Patterns/Pattern Files/zinc.FCMat"
)

SET(MaterialTestLib_Files
    "Resources/Materials/Test/Test Material.FCMat"
)

SET(MaterialModel_Files
    Resources/Models/Architectural/Architectural.yml
    Resources/Models/Architectural/ArchitecturalRendering.yml
    Resources/Models/Costs/Costs.yml
    Resources/Models/Electromagnetic/Electromagnetic.yml
    Resources/Models/Fluid/Fluid.yml
    Resources/Models/Legacy/Father.yml
    Resources/Models/Legacy/MaterialStandard.yml
    Resources/Models/Mechanical/ArrudaBoyce.yml
    Resources/Models/Mechanical/Density.yml
    Resources/Models/Mechanical/IsotropicLinearElastic.yml
    Resources/Models/Mechanical/LinearElastic.yml
    Resources/Models/Mechanical/MooneyRivlin.yml
    Resources/Models/Mechanical/NeoHooke.yml
    Resources/Models/Mechanical/OgdenN1.yml
    Resources/Models/Mechanical/OgdenN2.yml
    Resources/Models/Mechanical/OgdenN3.yml
    Resources/Models/Mechanical/OgdenYld2004p18.yml
    Resources/Models/Mechanical/OrthotropicLinearElastic.yml
    Resources/Models/Mechanical/PolynomialN1.yml
    Resources/Models/Mechanical/PolynomialN2.yml
    Resources/Models/Mechanical/PolynomialN3.yml
    Resources/Models/Mechanical/ReducedPolynomialN1.yml
    Resources/Models/Mechanical/ReducedPolynomialN2.yml
    Resources/Models/Mechanical/ReducedPolynomialN3.yml
    Resources/Models/Mechanical/Yeoh.yml
    Resources/Models/Patterns/PAT.yml
    "Resources/Models/Patterns/Pattern File.yml"
    "Resources/Models/Render Workbench/RenderAppleseed.yml"
    "Resources/Models/Render Workbench/RenderCarpaint.yml"
    "Resources/Models/Render Workbench/RenderCycles.yml"
    "Resources/Models/Render Workbench/RenderDiffuse.yml"
    "Resources/Models/Render Workbench/RenderDisney.yml"
    "Resources/Models/Render Workbench/RenderEmission.yml"
    "Resources/Models/Render Workbench/RenderGlass.yml"
    "Resources/Models/Render Workbench/RenderLuxcore.yml"
    "Resources/Models/Render Workbench/RenderLuxrender.yml"
    "Resources/Models/Render Workbench/RenderMixed.yml"
    "Resources/Models/Render Workbench/RenderOspray.yml"
    "Resources/Models/Render Workbench/RenderPbrt.yml"
    "Resources/Models/Render Workbench/RenderPovray.yml"
    "Resources/Models/Render Workbench/RenderSubstancePBR.yml"
    "Resources/Models/Render Workbench/RenderTexture.yml"
    "Resources/Models/Render Workbench/RenderWB.yml"
    Resources/Models/Rendering/AdvancedRendering.yml
    Resources/Models/Rendering/BasicRendering.yml
    Resources/Models/Rendering/TextureRendering.yml
    Resources/Models/Rendering/VectorRendering.yml
    "Resources/Models/Test/Test Model.yml"
    Resources/Models/Thermal/Thermal.yml
)

set(MaterialTest_Files
    materialtests/__init__.py
    materialtests/TestModels.py
    materialtests/TestMaterials.py
)

ADD_CUSTOM_TARGET(MateriaTestLib ALL
    SOURCES ${MaterialTest_Files}
)

fc_target_copy_resource(MateriaTestLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Material
    ${MaterialTest_Files})

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

ADD_CUSTOM_TARGET(MaterialLib ALL
    SOURCES ${MaterialLib_Files}
)
ADD_CUSTOM_TARGET(FluidMaterialLib ALL
    SOURCES ${FluidMaterial_Files}
)
ADD_CUSTOM_TARGET(AppearanceLib ALL
    SOURCES ${AppearanceLib_Files}
)
ADD_CUSTOM_TARGET(PatternLib ALL
    SOURCES ${PatternLib_Files}
)
ADD_CUSTOM_TARGET(MaterialTestLib ALL
    SOURCES ${MaterialTestLib_Files}
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
fc_target_copy_resource(AppearanceLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material/
    ${AppearanceLib_Files})
fc_target_copy_resource(PatternLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material/
    ${PatternLib_Files})
fc_target_copy_resource(MaterialTestLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material/
    ${MaterialTestLib_Files})
fc_target_copy_resource(MaterialModelLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material/
    ${MaterialModel_Files})

INSTALL(
    FILES ${Material_SRCS} ${Material_QRC_SRCS}
    DESTINATION Mod/Material
)

INSTALL(
    FILES ${MaterialTest_Files}
    DESTINATION Mod/Material/materialtests
)

foreach(file ${MaterialLib_Files} ${FluidMaterial_Files} ${AppearanceLib_Files} ${PatternLib_Files} ${MaterialTestLib_Files} ${MaterialModel_Files})
    get_filename_component(filepath ${file} DIRECTORY)
    INSTALL(
        FILES ${file}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Material/${filepath}
    )
endforeach()
