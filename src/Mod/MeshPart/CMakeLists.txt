# SPDX-License-Identifier: LGPL-2.1-or-later

if(OCC_VERSION_STRING VERSION_GREATER_EQUAL "8.0.0")
    add_definitions(-DOCCT_NO_DEPRECATED)
endif()

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
    DESTINATION
        Mod/MeshPart
)
