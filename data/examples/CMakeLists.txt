
SET(Examples_Files
    Schenkel.stp
    draft_test_objects.FCStd
    EngineBlock.FCStd
    PartDesignExample.FCStd
    RobotExample.FCStd
    ArchDetail.FCStd
    FemCalculixCantilever1D.FCStd
    FemCalculixCantilever2D.FCStd
    FemCalculixCantilever3D.FCStd
    AssemblyExample.FCStd
)

ADD_CUSTOM_TARGET(Example_data ALL
    SOURCES ${Examples_Files}
)

# 0001097: CMake stops with error "Circular ... <- ... dependency dropped."
if(NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    fc_copy_sources(Example_data "${CMAKE_BINARY_DIR}/data/examples" ${Examples_Files})
endif()

if (WIN32 AND FREECAD_LIBPACK_USE)
    # pivy
    foreach (it ${FREECAD_LIBPACK_PIVY_COIN})
        get_filename_component(libfile ${it} NAME)
        set(out "${CMAKE_BINARY_DIR}/lib/pivy/${libfile}")
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

    # shiboken
    foreach (it ${FREECAD_LIBPACK_SHIBOKEN})
        get_filename_component(libfile ${it} NAME)
        set(out "${CMAKE_BINARY_DIR}/lib/${libfile}")
        add_custom_command(
            DEPENDS   ${it}
            OUTPUT    ${out}
            COMMAND   ${CMAKE_COMMAND}
            ARGS      -E copy_if_different ${it} ${out}
            MAIN_DEPENDENCY ${it}
            COMMENT   "Copy file to ${out}"
    )
    endforeach (it)
    SOURCE_GROUP("Shiboken" FILES ${FREECAD_LIBPACK_SHIBOKEN})

    # PySide
    foreach (it ${FREECAD_LIBPACK_PYSIDE})
        get_filename_component(libfile ${it} NAME)
        set(out "${CMAKE_BINARY_DIR}/lib/PySide/${libfile}")
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

    # PySide tools
    foreach (it ${FREECAD_LIBPACK_PYSIDEUIC})
        get_filename_component(libpath ${it} PATH)
        set(src "${FREECAD_LIBPACK_PYSIDEUIC_REL}/${it}")
        if("${libpath}" STREQUAL "pysideuic")
            set(FREECAD_LIBPACK_PYSIDEUIC_TOP ${FREECAD_LIBPACK_PYSIDEUIC_TOP} ${src})
        elseif("${libpath}" STREQUAL "pysideuic/Compiler")
            set(FREECAD_LIBPACK_PYSIDEUIC_CMP ${FREECAD_LIBPACK_PYSIDEUIC_CMP} ${src})
        elseif("${libpath}" STREQUAL "pysideuic/port_v2")
            set(FREECAD_LIBPACK_PYSIDEUIC_PV2 ${FREECAD_LIBPACK_PYSIDEUIC_PV2} ${src})
        elseif("${libpath}" STREQUAL "pysideuic/port_v3")
            set(FREECAD_LIBPACK_PYSIDEUIC_PV3 ${FREECAD_LIBPACK_PYSIDEUIC_PV3} ${src})
        elseif("${libpath}" STREQUAL "pysideuic/widget-plugins")
            set(FREECAD_LIBPACK_PYSIDEUIC_WPL ${FREECAD_LIBPACK_PYSIDEUIC_WPL} ${src})
        else()
            set(FREECAD_LIBPACK_PYSIDEUIC_OTH ${FREECAD_LIBPACK_PYSIDEUIC_OTH} ${src})
        endif()
        set(FREECAD_LIBPACK_PYSIDEUIC_ABS ${FREECAD_LIBPACK_PYSIDEUIC_ABS} ${src})
        set(out "${CMAKE_BINARY_DIR}/lib/${it}")
        add_custom_command(
            DEPENDS   ${src}
            OUTPUT    ${out}
            COMMAND   ${CMAKE_COMMAND}
            ARGS      -E copy_if_different ${src} ${out}
            MAIN_DEPENDENCY ${src}
            COMMENT   "Copy file to ${out}"
    )
    endforeach (it)
    SOURCE_GROUP("PySide tools" FILES ${FREECAD_LIBPACK_PYSIDEUIC_OTH})
    SOURCE_GROUP("PySide tools\\pysideuic" FILES ${FREECAD_LIBPACK_PYSIDEUIC_TOP})
    SOURCE_GROUP("PySide tools\\pysideuic\\Compiler" FILES ${FREECAD_LIBPACK_PYSIDEUIC_CMP})
    SOURCE_GROUP("PySide tools\\pysideuic\\port_v2" FILES ${FREECAD_LIBPACK_PYSIDEUIC_PV2})
    SOURCE_GROUP("PySide tools\\pysideuic\\port_v3" FILES ${FREECAD_LIBPACK_PYSIDEUIC_PV3})
    SOURCE_GROUP("PySide tools\\pysideuic\\widget-plugins" FILES ${FREECAD_LIBPACK_PYSIDEUIC_WPL})

    # Python extension modules
    foreach (it ${FREECAD_LIBPACK_PYTHON})
        get_filename_component(libfile ${it} NAME)
        set(out "${CMAKE_BINARY_DIR}/lib/${libfile}")
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

    set(FREECAD_LIBPACK_FILES
        ${FREECAD_LIBPACK_PIVY_COIN}
        ${FREECAD_LIBPACK_SHIBOKEN}
        ${FREECAD_LIBPACK_PYSIDE}
        ${FREECAD_LIBPACK_PYSIDEUIC_ABS}
        ${FREECAD_LIBPACK_PYTHON}
    )
    add_custom_target(LibPack_Files ALL SOURCES ${FREECAD_LIBPACK_FILES})

endif(WIN32 AND FREECAD_LIBPACK_USE)

INSTALL(
    FILES
        ${Examples_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/examples
)

