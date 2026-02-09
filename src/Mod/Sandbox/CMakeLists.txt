# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Sandbox_Scripts
    Init.py
    exportDRAWEXE.py
)

if(BUILD_GUI)
    list (APPEND Sandbox_Scripts InitGui.py)
endif(BUILD_GUI)

INSTALL(
    FILES
        ${Sandbox_Scripts}
    DESTINATION
        Mod/Sandbox
)
