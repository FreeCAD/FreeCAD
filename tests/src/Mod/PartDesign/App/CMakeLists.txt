
target_sources(
        PartDesign_tests_run
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/BackwardCompatibility.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DatumPlane.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ShapeBinder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Pad.cpp
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
