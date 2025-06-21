target_sources(Sketcher_tests_run PRIVATE
        Constraint.cpp
        SketcherTestHelpers.cpp
        SketchObject.cpp
        SketchObjectChanges.cpp
)

add_subdirectory(planegcs)
