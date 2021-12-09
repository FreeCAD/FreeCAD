
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(_TEMPLATE__Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND _TEMPLATE__Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(_TEMPLATE_Scripts ALL
    SOURCES ${_TEMPLATE__Scripts}
)

fc_target_copy_resource_flat(_TEMPLATE_Scripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/_TEMPLATE_
    ${_TEMPLATE__Scripts}
)

install(
    FILES
        ${_TEMPLATE__Scripts}
    DESTINATION
        Mod/_TEMPLATE_
)
