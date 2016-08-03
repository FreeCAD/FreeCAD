if(MSVC)
    add_definitions(-DFCAppMeasure -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H -DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(Measure_LIBS
    Part
    FreeCADApp
)

generate_from_xml(MeasurementPy)

SET(Python_SRCS
    MeasurementPy.xml
    MeasurementPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(MeasureModule_SRCS
    AppMeasure.cpp
    AppMeasurePy.cpp
    Measurement.cpp
    Measurement.h
    PreCompiled.cpp
    PreCompiled.h
)

SOURCE_GROUP("Module" FILES ${MeasureModule_SRCS})


SET(Measure_SRCS
    ${MeasureModule_SRCS}
    ${Python_SRCS}
)

if(MSVC)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" Measure_CPP_SRCS ${Measure_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Measure PreCompiled.h PreCompiled.cpp Measure_CPP_SRCS)
endif(MSVC)

# Set special compiler flag to convert a SIGSEV into an exception
# to handle issue #0000478.
IF(MSVC)
ENDIF(MSVC)

add_library(Measure SHARED ${Measure_SRCS} )
target_link_libraries(Measure ${Measure_LIBS})

SET(Measure_Scripts
    Init.py
)

fc_target_copy_resource(Measure 
    ${CMAKE_SOURCE_DIR}/src/Mod/Measure
    ${CMAKE_BINARY_DIR}/Mod/Measure
    ${Measure_Scripts})

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
