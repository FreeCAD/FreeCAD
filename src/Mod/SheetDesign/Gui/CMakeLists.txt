# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# The GUI logic of Sheet Design is in Python. This library holds the resources that the Python
# code uses: the icons, and the .ui files of the task panels (Resources/panels).

set(SheetDesignGui_LIBS
    SheetDesign
    FreeCADGui
)

qt_add_resources(SheetDesignGui_QRC_SRCS
    Resources/SheetDesign.qrc
    OPTIONS ${FREECAD_RCC_OPTIONS}
)

SET(SheetDesignGui_SRCS
    ${SheetDesignGui_QRC_SRCS}
    AppSheetDesignGui.cpp
    Resources/SheetDesign.qrc
)

# The icon of the workbench is also a file, because the workbench list needs it before this
# library is loaded.
SET(SheetDesignGuiIcon_SVG
    Resources/icons/SheetDesignWorkbench.svg
)

add_library(SheetDesignGui SHARED
    ${SheetDesignGui_SRCS}
    ${SheetDesignGuiIcon_SVG}
)

target_include_directories(
    SheetDesignGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_directories(SheetDesignGui PUBLIC ${OCC_LIBRARY_DIR})
target_link_libraries(SheetDesignGui ${SheetDesignGui_LIBS})
target_compile_warn_error(SheetDesignGui)

SET_BIN_DIR(SheetDesignGui)
SET_PYTHON_PREFIX_SUFFIX(SheetDesignGui)

fc_copy_sources(SheetDesignGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/SheetDesign" ${SheetDesignGuiIcon_SVG})

install(TARGETS SheetDesignGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${SheetDesignGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/SheetDesign/Resources/icons")
