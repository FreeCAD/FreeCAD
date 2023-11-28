
target_sources(
    Material_tests_run
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMaterialCards.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMaterialProperties.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMaterials.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMaterialValue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestModelProperties.cpp
)

target_include_directories(Material_tests_run PUBLIC
    ${QtCore_INCLUDE_DIRS}
)
