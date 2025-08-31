add_executable(Mesh_tests_run
        Core/KDTree.cpp
        Exporter.cpp
        Importer.cpp
        Mesh.cpp
        MeshFeature.cpp
)

target_compile_definitions(Mesh_tests_run PRIVATE DATADIR="${CMAKE_SOURCE_DIR}/data")
