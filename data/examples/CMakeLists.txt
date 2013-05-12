
SET(Examples_Files 
    Schenkel.stp
    DrawingExample.FCStd
    EngineBlock.FCStd
    PartDesignExample.FCStd
    RobotExample.FCStd
    ArchDetail.FCStd
)

ADD_CUSTOM_TARGET(Example_data ALL
    SOURCES ${Examples_Files}
)

# 0001097: CMake stops with error "Circular ... <- ... dependency dropped."
if(NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    fc_copy_sources(Examples "${CMAKE_BINARY_DIR}/data/examples" ${Examples_Files})
endif()

INSTALL(
    FILES
        ${Examples_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/examples
)

