add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Export3DPDF_Scripts
    Init.py
)

if(BUILD_GUI)
    list(APPEND Export3DPDF_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(Export3DPDFScripts ALL
    SOURCES ${Export3DPDF_Scripts}
)

fc_target_copy_resource(Export3DPDFScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Export3DPDF
    ${Export3DPDF_Scripts}
)

INSTALL(
    FILES
        ${Export3DPDF_Scripts}
    DESTINATION
        Mod/Export3DPDF
) 