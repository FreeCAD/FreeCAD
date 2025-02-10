target_sources(
    Sketcher_tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/SketcherTestHelpers.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/SketchObject.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/SketchObjectChanges.cpp
)

add_subdirectory(planegcs)
