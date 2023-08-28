target_sources(
    Sketcher_tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/SketchObject.cpp
)

add_subdirectory(planegcs)
