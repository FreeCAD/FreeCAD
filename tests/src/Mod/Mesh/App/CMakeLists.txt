target_sources(
    Mesh_tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Core/KDTree.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Exporter.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cpp
)
