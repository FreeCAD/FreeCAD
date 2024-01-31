
add_subdirectory(App)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)


set(Measure_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND Measure_Scripts InitGui.py)
endif(BUILD_GUI)


INSTALL(
    FILES
        ${Measure_Scripts}
    DESTINATION
        Mod/Measure
)

ADD_CUSTOM_TARGET(MeasureScripts ALL
    SOURCES ${Measure_Scripts}
)

fc_copy_sources(MeasureScripts "${CMAKE_BINARY_DIR}/Mod/Measure" ${Measure_Scripts})