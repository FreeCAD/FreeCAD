
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Mesh_Scripts
    Init.py
    BuildRegularGeoms.py
    App/MeshTestsApp.py
)

if(BUILD_GUI)
    list (APPEND Mesh_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(MeshScripts ALL
    SOURCES ${Mesh_Scripts}
)

fc_target_copy_resource_flat(MeshScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Mesh
    ${Mesh_Scripts}
)

INSTALL(
    FILES
        ${Mesh_Scripts}
    DESTINATION
        Mod/Mesh
)
