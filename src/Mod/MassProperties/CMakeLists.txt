# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

add_subdirectory(App)

set(MassProperties_Scripts
    Init.py
)

if(BUILD_GUI)
    list(APPEND MassProperties_Scripts InitGui.py)
    add_subdirectory(Gui)
endif(BUILD_GUI)

add_custom_target(MassPropertiesScripts ALL
    SOURCES ${MassProperties_Scripts}
)

fc_target_copy_resource(MassPropertiesScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/MassProperties
    ${MassProperties_Scripts})

INSTALL(
    FILES
    ${MassProperties_Scripts}
    DESTINATION
        Mod/MassProperties
)