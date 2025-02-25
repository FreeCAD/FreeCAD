target_sources(MeshPart_tests_run PRIVATE
        MeshPart.cpp
)

target_include_directories(MeshPart_tests_run PUBLIC
        ${CMAKE_BINARY_DIR}
)
