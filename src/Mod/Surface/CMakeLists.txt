
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Surface_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND Surface_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(SurfaceScripts ALL
    SOURCES ${Surface_Scripts}
)

fc_target_copy_resource(SurfaceScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Surface
    ${Surface_Scripts}
)

install(
    FILES
        ${Surface_Scripts}
    DESTINATION
        Mod/Surface
)
