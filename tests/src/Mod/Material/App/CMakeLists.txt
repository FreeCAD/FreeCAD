add_executable(Material_tests_run
        TestMaterialCards.cpp
        TestMaterialFilter.cpp
        TestMaterialProperties.cpp
        TestMaterials.cpp
        TestMaterialValue.cpp
        TestModel.cpp
        TestModelProperties.cpp
)

target_include_directories(Material_tests_run SYSTEM PUBLIC
        ${QtCore_INCLUDE_DIRS}
)

set(MaterialTestData_Files
        Materials/TestAcrylicLegacy.FCMat
        Materials/TestAluminumAppearance.FCMat
        Materials/TestAluminumMixed.FCMat
        Materials/TestAluminumPhysical.FCMat
        Materials/TestBrassAppearance.FCMat
)

ADD_CUSTOM_TARGET(MaterialTestData ALL
        SOURCES ${MaterialTestData_Files}
)

fc_target_copy_resource(MaterialTestData
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/tests
        ${MaterialTestData_Files})

# INSTALL(
#     FILES ${MaterialTest_Files}
#     DESTINATION Mod/Material/materialtests
# )
