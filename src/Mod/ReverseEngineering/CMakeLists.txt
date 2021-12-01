
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Reen_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND Reen_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(ReverseEngineeringScripts ALL
    SOURCES ${Reen_Scripts}
)

fc_target_copy_resource(ReverseEngineeringScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/ReverseEngineering
    ${Reen_Scripts}
)

install(
    FILES
        ${Reen_Scripts}
    DESTINATION
        Mod/ReverseEngineering
)
