
SET(Material_SRCS
    Init.py
    InitGui.py
    Material.py
    importFCMat.py
)
SOURCE_GROUP("" FILES ${Material_SRCS})

# collect all the material cards:
#FILE( GLOB MaterialLib_Files ./StandardMaterial/*.FCMat ./StandardMaterial/*.txt  )

SET (MaterialLib_Files
    Steel.FCMat
    ABS.FCMat
    PLA.FCMat
	Readme.txt
	)

SET(all_files ${Material_SRCS})

ADD_CUSTOM_TARGET(Material ALL
    SOURCES ${all_files}
)

fc_copy_sources(Material "${CMAKE_BINARY_DIR}/Mod/Material" ${all_files})

fc_target_copy_resource(Material 
    ${CMAKE_SOURCE_DIR}/src/Mod/Material/StandardMaterial
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material/StandardMaterial
    ${MaterialLib_Files})

INSTALL(
    FILES ${Material_SRCS}
    DESTINATION Mod/Material
)
