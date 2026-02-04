# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(MassPropertiesApp_LIBS
	FreeCADGui
	Part
)

SET(MassPropertiesApp_SRCS
	${MassPropertiesApp_QRC_SRCS}
    MassPropertiesResult.cpp
    MassPropertiesResult.h
)


add_library(MassPropertiesApp SHARED ${MassPropertiesApp_SRCS})

target_include_directories(
	MassPropertiesApp
	PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(MassPropertiesApp ${MassPropertiesApp_LIBS})

if (FREECAD_WARN_ERROR)
	target_compile_warn_error(MassPropertiesApp)
endif()

SET_BIN_DIR(MassPropertiesApp MassPropertiesApp /Mod/MassProperties)
SET_PYTHON_PREFIX_SUFFIX(MassPropertiesApp)
INSTALL(TARGETS MassPropertiesApp DESTINATION ${CMAKE_INSTALL_LIBDIR})