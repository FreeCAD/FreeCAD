include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
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

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" Measure_CPP_SRCS ${Measure_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Measure PreCompiled.h PreCompiled.cpp Measure_CPP_SRCS)
endif(FREECAD_USE_PCH)

SET(Measure_Scripts
    ../Init.py
)

add_library(Measure SHARED ${Measure_SRCS} ${Measure_Scripts})
target_link_libraries(Measure ${Measure_LIBS})

fc_target_copy_resource_flat(Measure
    ${CMAKE_CURRENT_SOURCE_DIR}
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
