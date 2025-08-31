include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(Measure_LIBS
    Part
    FreeCADApp
)

generate_from_xml(MeasurementPy)
generate_from_xml(MeasureBasePy)

generate_from_py_(MeasurementPy)
generate_from_py_(MeasureBasePy)

SET(Python_SRCS
    MeasurementPy.xml
    MeasurementPyImp.cpp
    MeasureBasePy.xml
    MeasureBasePyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(MeasureModule_SRCS
    PreCompiled.cpp
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

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" Measure_CPP_SRCS ${Measure_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Measure PreCompiled.h PreCompiled.cpp Measure_CPP_SRCS)
endif(FREECAD_USE_PCH)

add_library(Measure SHARED ${Measure_SRCS})
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
