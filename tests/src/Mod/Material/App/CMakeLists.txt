
target_sources(
    Material_tests_run
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMaterialCards.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMaterialFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMaterialProperties.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMaterials.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMaterialValue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestModelProperties.cpp
)

target_include_directories(Material_tests_run PUBLIC
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
