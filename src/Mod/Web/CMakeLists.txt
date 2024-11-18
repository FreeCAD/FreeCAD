add_subdirectory(App)

set(Web_Scripts
    Init.py
)

add_custom_target(WebScripts ALL
    SOURCES ${Web_Scripts}
)

fc_target_copy_resource_flat(WebScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Web
    ${Web_Scripts}
)

INSTALL(
    FILES
        ${Web_Scripts}
    DESTINATION
        Mod/Web
)
