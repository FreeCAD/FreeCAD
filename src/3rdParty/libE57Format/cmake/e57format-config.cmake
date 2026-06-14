include(CMakeFindDependencyMacro)

find_dependency(XercesC REQUIRED)
include(${CMAKE_CURRENT_LIST_DIR}/E57Format-export.cmake)

set_target_properties(E57Format PROPERTIES
    MAP_IMPORTED_CONFIG_MINSIZEREL "MinSizeRel;Release"
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO "RelWithDebInfo;Release"
)
