# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(Measure_LIBS
    Part
    FreeCADApp
)

generate_from_py(Measurement)
generate_from_py(MeasureBase)

SET(Python_SRCS
    Measurement.pyi
    MeasurementPyImp.cpp
    MeasureBase.pyi
    MeasureBasePyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(MeasureModule_SRCS
    PreCompiled.h
    AppMeasure.cpp
    AppMeasurePy.cpp

# original service routines
    Measurement.cpp
    Measurement.h

# umf
    MeasureBase.cpp
    MeasureBase.h
    MeasureAngle.cpp
    MeasureAngle.h
    MeasureDistance.cpp
    MeasureDistance.h
    MeasurePosition.cpp
    MeasurePosition.h
    MeasureLength.cpp
    MeasureLength.h
    MeasureArea.cpp
    MeasureArea.h
    MeasureDiameter.cpp
    MeasureDiameter.h
    MeasureRadius.cpp
    MeasureRadius.h

    Preferences.cpp
    Preferences.h

    ShapeFinder.cpp
    ShapeFinder.h
    SubnameHelper.cpp
    SubnameHelper.h
)

SOURCE_GROUP("Module" FILES ${MeasureModule_SRCS})

SET(Measure_SRCS
    ${MeasureModule_SRCS}
    ${Python_SRCS}
)

add_library(Measure SHARED ${Measure_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Measure PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    Measure
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(Measure ${Measure_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Measure)
endif()

#if(MSVC)
#    set_target_properties(Measure PROPERTIES SUFFIX ".pyd")
#    set_target_properties(Measure PROPERTIES DEBUG_OUTPUT_NAME "Measure_d")
#    set_target_properties(Measure PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Measure)
#    set_target_properties(Measure PROPERTIES PREFIX "../")
#elseif(MINGW)
#    set_target_properties(Measure PROPERTIES SUFFIX ".pyd")
#    set_target_properties(Measure PROPERTIES DEBUG_OUTPUT_NAME "Measure_d")
#    set_target_properties(Measure PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Measure)
#    set_target_properties(Measure PROPERTIES PREFIX "")
#else(MSVC)
#    set_target_properties(Measure PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Measure)
#    set_target_properties(Measure PROPERTIES PREFIX "")
#    set_target_properties(Measure PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
#endif(MSVC)

SET_BIN_DIR(Measure Measure /Mod/Measure)
SET_PYTHON_PREFIX_SUFFIX(Measure)

INSTALL(TARGETS Measure DESTINATION ${CMAKE_INSTALL_LIBDIR})
