# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Sketcher3D_Scripts
    Init.py
)

if(BUILD_GUI)
    list(APPEND Sketcher3D_Scripts
        InitGui.py
    )
endif(BUILD_GUI)

add_custom_target(Sketcher3DScripts ALL
    SOURCES ${Sketcher3D_Scripts}
)

fc_target_copy_resource(Sketcher3DScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Sketcher3D
    ${Sketcher3D_Scripts}
)

INSTALL(
    FILES ${Sketcher3D_Scripts}
    DESTINATION Mod/Sketcher3D
)
