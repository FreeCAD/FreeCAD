target_sources(Sketcher_tests_run PRIVATE
        SketcherTestHelpers.cpp
        SketchObject.cpp
        SketchObjectChanges.cpp
)

add_subdirectory(planegcs)
