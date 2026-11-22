# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

SET(PlotMain_SRCS
    Plot.py
)
SOURCE_GROUP("" FILES ${PlotMain_SRCS})

SET(all_files ${PlotMain_SRCS})

ADD_CUSTOM_TARGET(Plot ALL
    SOURCES ${all_files}
)

fc_copy_sources(Plot "${CMAKE_BINARY_DIR}/Mod/Plot" ${all_files})

INSTALL(
    FILES
        ${PlotMain_SRCS}
    DESTINATION
        Mod/Plot
)
