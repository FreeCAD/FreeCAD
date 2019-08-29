
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Raytracing_Scripts
    Init.py
    RaytracingExample.py
)

if(BUILD_GUI)
    list (APPEND Raytracing_Scripts InitGui.py)
endif(BUILD_GUI)

SET(Raytracing_Templates
    Templates/LuxClassic.lxs
    Templates/LuxOutdoor.lxs
    Templates/ProjectStd.pov
    Templates/RadiosityNormal.pov
    Templates/RadiosityOutdoorHQ.pov
)

add_custom_target(RaytracingScripts ALL
    SOURCES ${Raytracing_Scripts} ${Raytracing_Templates}
)

fc_target_copy_resource(RaytracingScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Raytracing
    ${Raytracing_Scripts}
)

fc_target_copy_resource(RaytracingScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Raytracing
    ${Raytracing_Templates}
)

INSTALL(
    FILES
        ${Raytracing_Scripts}
    DESTINATION
        Mod/Raytracing
)

INSTALL(
    FILES
        ${Raytracing_Templates}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Raytracing/Templates
)

