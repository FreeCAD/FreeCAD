
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Image_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND Image_Scripts InitGui.py)
    set(Image_ToolsScripts
        ImageTools/__init__.py
        ImageTools/_CommandImageScaling.py
    )
endif(BUILD_GUI)

add_custom_target(ImageScripts ALL
    SOURCES ${Image_Scripts} ${Image_ToolsScripts}
)

fc_target_copy_resource(ImageScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Image
    ${Image_Scripts}
    ${Image_ToolsScripts}
)

INSTALL(
    FILES
        ${Image_Scripts}
    DESTINATION
        Mod/Image
)

INSTALL(
    FILES
        ${Image_ToolsScripts}
    DESTINATION
        Mod/Image/ImageTools
)
