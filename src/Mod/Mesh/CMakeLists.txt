
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Mesh_Scripts
    Init.py
    BuildRegularGeoms.py
    MeshEnums.py
    App/MeshTestsApp.py
)

set(MeshTestDataFiles
    App/TestData/NASTRAN_Test_GRID_CQUAD4.bdf
    App/TestData/NASTRAN_Test_Delimited_GRID_CQUAD4.bdf
    App/TestData/NASTRAN_Test_GRIDSTAR_CQUAD4.bdf
    App/TestData/NASTRAN_Test_GRID_CTRIA3.bdf
)

if(BUILD_GUI)
    list (APPEND Mesh_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(MeshScripts ALL
    SOURCES ${Mesh_Scripts}
)

add_custom_target(MeshTestData ALL
    SOURCES ${MeshTestDataFiles}
)

fc_target_copy_resource_flat(MeshScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Mesh
    ${Mesh_Scripts}
)

fc_copy_sources(MeshTestData "${CMAKE_BINARY_DIR}/Mod/Mesh/" ${MeshTestDataFiles})

INSTALL(
    FILES
        ${Mesh_Scripts}
    DESTINATION
        Mod/Mesh
)


INSTALL(FILES ${MeshTestDataFiles} DESTINATION Mod/Mesh/App/TestData)
