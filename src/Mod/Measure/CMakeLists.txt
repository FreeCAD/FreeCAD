
add_subdirectory(App)

set(Measure_Scripts
    Init.py
    UtilsMeasure.py
    MeasureCOM.py
)

if(BUILD_GUI)
    list(APPEND Measure_Scripts InitGui.py)
    add_subdirectory(Gui)
endif(BUILD_GUI)

add_custom_target(MeasureScripts ALL
    SOURCES ${Measure_Scripts}
)

fc_target_copy_resource(MeasureScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Measure
    ${Measure_Scripts})

INSTALL(
    FILES
    ${Measure_Scripts}
    DESTINATION
        Mod/Measure
)
