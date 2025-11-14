# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)
if(BUILD_GUI)
    #add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
    DESTINATION
        Mod/JtReader
)
