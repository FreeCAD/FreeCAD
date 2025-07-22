
add_executable(PartDesign_tests_run
        BackwardCompatibility.cpp
        DatumPlane.cpp
        ShapeBinder.cpp
        Pad.cpp
)

set(PartDesignTestData_Files
        TestModels/ModelFromV021.FCStd
)

ADD_CUSTOM_TARGET(PartDesignTestData ALL
        SOURCES ${PartDesignTestData_Files}
)

fc_target_copy_resource(PartDesignTestData
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/tests
        ${PartDesignTestData_Files}
)
