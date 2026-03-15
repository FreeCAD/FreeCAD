# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(MassPropertiesGui_LIBS
    FreeCADGui
    Part
    PartDesign
    ${OCC_LIBRARIES}
)

qt_add_resources(MassPropertiesGui_QRC_SRCS Resources/MassProperties.qrc)

set(MassPropertiesGui_SRCS
    ${MassPropertiesGui_QRC_SRCS}
    AppMassPropertiesGui.cpp
    Command.cpp
    TaskMassProperties.cpp
    UnitHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../App/MassPropertiesResult.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../App/MassPropertiesResult.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../App/MassPropertiesObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../App/MassPropertiesObject.h
    ViewProviderMassPropertiesResult.cpp
    ViewProviderMassPropertiesResult.h
)

add_library(MassPropertiesGui SHARED ${MassPropertiesGui_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(MassPropertiesGui PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif()

target_include_directories(
    MassPropertiesGui
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${OCC_INCLUDE_DIR}
)

target_link_libraries(MassPropertiesGui ${MassPropertiesGui_LIBS})

if(FREECAD_WARN_ERROR)
    target_compile_warn_error(MassPropertiesGui)
endif()

SET_BIN_DIR(MassPropertiesGui MassPropertiesGui /Mod/MassProperties)
SET_PYTHON_PREFIX_SUFFIX(MassPropertiesGui)

INSTALL(TARGETS MassPropertiesGui DESTINATION ${CMAKE_INSTALL_LIBDIR})