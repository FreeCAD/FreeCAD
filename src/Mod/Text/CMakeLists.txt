
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Text_Scripts
    Init.py
    TextExample.py
)

if(BUILD_GUI)
    list (APPEND Text_Scripts
          InitGui.py
    )
endif(BUILD_GUI)

add_custom_target(TextScripts ALL
    SOURCES ${Text_Scripts}
)

fc_target_copy_resource(TextScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Text
    ${Text_Scripts}
)

INSTALL(
    FILES
        ${Text_Scripts}
    DESTINATION
        Mod/Text
)
