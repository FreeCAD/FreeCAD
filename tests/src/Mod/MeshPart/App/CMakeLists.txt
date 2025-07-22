add_executable(MeshPart_tests_run
        MeshPart.cpp
)

target_include_directories(MeshPart_tests_run PUBLIC
        ${CMAKE_BINARY_DIR}
)
