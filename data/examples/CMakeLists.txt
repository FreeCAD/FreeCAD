
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
    fc_copy_sources(Example_data "${CMAKE_BINARY_DIR}/data/examples" ${Examples_Files})
endif()

if (WIN32 AND FREECAD_LIBPACK_USE)
    set(FREECAD_LIBPACK_FILES ${FREECAD_LIBPACK_PIVY_COIN} ${FREECAD_LIBPACK_PIVY_SOQT} ${FREECAD_LIBPACK_PYSIDE} ${FREECAD_LIBPACK_PYTHON})
    add_custom_target(LibPack_Files ALL SOURCES ${FREECAD_LIBPACK_FILES})

    # pivy
    foreach (it ${FREECAD_LIBPACK_PIVY_COIN})
        get_filename_component(libfile ${it} NAME)
        set(out "${CMAKE_BINARY_DIR}/DLLs/pivy/${libfile}")
        add_custom_command(
            DEPENDS   ${it}
            OUTPUT    ${out}
            COMMAND   ${CMAKE_COMMAND}
            ARGS      -E copy_if_different ${it} ${out}
            MAIN_DEPENDENCY ${it}
            COMMENT   "Copy file to ${out}"
    )
    endforeach (it)
    SOURCE_GROUP("pivy" FILES ${FREECAD_LIBPACK_PIVY_COIN})

    # pivy/gui
    foreach (it ${FREECAD_LIBPACK_PIVY_SOQT})
        get_filename_component(libfile ${it} NAME)
        set(out "${CMAKE_BINARY_DIR}/DLLs/pivy/gui/${libfile}")
        add_custom_command(
            DEPENDS   ${it}
            OUTPUT    ${out}
            COMMAND   ${CMAKE_COMMAND}
            ARGS      -E copy_if_different ${it} ${out}
            MAIN_DEPENDENCY ${it}
            COMMENT   "Copy file to ${out}"
    )
    endforeach (it)
    SOURCE_GROUP("pivy\\gui" FILES ${FREECAD_LIBPACK_PIVY_SOQT})

    # PySide
    foreach (it ${FREECAD_LIBPACK_PYSIDE})
        get_filename_component(libfile ${it} NAME)
        set(out "${CMAKE_BINARY_DIR}/DLLs/PySide/${libfile}")
        add_custom_command(
            DEPENDS   ${it}
            OUTPUT    ${out}
            COMMAND   ${CMAKE_COMMAND}
            ARGS      -E copy_if_different ${it} ${out}
            MAIN_DEPENDENCY ${it}
            COMMENT   "Copy file to ${out}"
    )
    endforeach (it)
    SOURCE_GROUP("PySide" FILES ${FREECAD_LIBPACK_PYSIDE})

    # Python extension modules
    foreach (it ${FREECAD_LIBPACK_PYTHON})
        get_filename_component(libfile ${it} NAME)
        set(out "${CMAKE_BINARY_DIR}/DLLs/${libfile}")
        add_custom_command(
            DEPENDS   ${it}
            OUTPUT    ${out}
            COMMAND   ${CMAKE_COMMAND}
            ARGS      -E copy_if_different ${it} ${out}
            MAIN_DEPENDENCY ${it}
            COMMENT   "Copy file to ${out}"
    )
    endforeach (it)
    SOURCE_GROUP("Python" FILES ${FREECAD_LIBPACK_PYTHON})

endif(WIN32 AND FREECAD_LIBPACK_USE)

INSTALL(
    FILES
        ${Examples_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/examples
)

