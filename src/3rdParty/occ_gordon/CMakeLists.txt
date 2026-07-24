add_library(occ_gordon_single INTERFACE)

target_include_directories(occ_gordon_single INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/3rdParty/occ_gordon>
)

target_sources(occ_gordon_single INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/occ_gordon_single.hpp>
)
install(FILES occ_gordon_single.hpp DESTINATION include/3rdParty/occ_gordon)