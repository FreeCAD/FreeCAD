# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(Base)
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif()
add_subdirectory(Mod)
add_subdirectory(zipios++)
