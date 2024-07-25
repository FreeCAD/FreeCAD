target_sources(
    MeshPart_tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/MeshPart.cpp
)

target_include_directories(
    MeshPart_tests_run
        PUBLIC
            ${CMAKE_BINARY_DIR}
)
