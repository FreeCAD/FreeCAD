# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

add_subdirectory(App)
add_subdirectory(AppGL)
#if(BUILD_GUI)
#     add_subdirectory(Gui)
#endif(BUILD_GUI)

# install(
    # FILES
        # Gui/PathSimulator.py
    # DESTINATION
        # Mod/CAM
# )
