
SET(Examples_Files 
    Schenkel.stp
    DrawingExample.FCStd
    EngineBlock.FCStd
    PartDesignExample.FCStd
    RobotExample.FCStd
)

ADD_CUSTOM_TARGET(Example_data ALL
    SOURCES ${Examples_Files}
)

fc_copy_sources("data/examples" "Examples" ${Examples_Files})

INSTALL(
    FILES
        ${Examples_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/examples
)

