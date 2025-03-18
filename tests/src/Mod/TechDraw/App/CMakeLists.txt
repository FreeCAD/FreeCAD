target_sources(TechDraw_tests_run PRIVATE
    GeometryId.cpp
)

target_include_directories(TechDraw_tests_run PUBLIC
    ${CMAKE_BINARY_DIR}
)
