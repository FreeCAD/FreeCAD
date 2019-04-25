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
    StandardMaterial/CalculiX-Steel.FCMat
    StandardMaterial/Concrete-Generic.FCMat
    StandardMaterial/Glass-Generic.FCMat
    StandardMaterial/PA6-Generic.FCMat
    StandardMaterial/PET-Generic.FCMat
    StandardMaterial/PLA-Generic.FCMat
    StandardMaterial/PP-Generic.FCMat
    StandardMaterial/PTFE-Generic.FCMat
    StandardMaterial/PVC-Generic.FCMat
    StandardMaterial/Steel-Generic.FCMat
    StandardMaterial/TEMPLATE.FCMat
    StandardMaterial/Wood-Generic.FCMat
    StandardMaterial/Readme.txt
)
SOURCE_GROUP("MatLib" FILES ${MaterialLib_Files})

SET (FluidMaterial_Files
    FluidMaterial/None.FCMat
    FluidMaterial/Air.FCMat
    FluidMaterial/Water.FCMat
    FluidMaterial/Readme.txt
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
