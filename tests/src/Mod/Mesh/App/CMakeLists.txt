target_compile_definitions(Mesh_tests_run PRIVATE DATADIR="${CMAKE_SOURCE_DIR}/data")

target_sources(
    Mesh_tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Core/KDTree.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Exporter.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Importer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/MeshFeature.cpp
)
