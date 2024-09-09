include_directories (
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS})
link_directories (${OCC_LIBRARY_DIR})

set (Measure_LIBS Part FreeCADApp)

generate_from_xml (MeasurementPy)
generate_from_xml (MeasureBasePy)

set (Python_SRCS MeasurementPy.xml MeasurementPyImp.cpp MeasureBasePy.xml MeasureBasePyImp.cpp)
source_group ("Python" FILES ${Python_SRCS})

set (
    MeasureModule_SRCS
    PreCompiled.cpp
    PreCompiled.h
    AppMeasure.cpp
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
    Preferences.h)

source_group ("Module" FILES ${MeasureModule_SRCS})

set (Measure_SRCS ${MeasureModule_SRCS} ${Python_SRCS})

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" Measure_CPP_SRCS ${Measure_SRCS})
    add_msvc_precompiled_header (Measure PreCompiled.h PreCompiled.cpp Measure_CPP_SRCS)
endif (FREECAD_USE_PCH)

add_library (Measure SHARED ${Measure_SRCS})
target_link_libraries (Measure ${Measure_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (Measure)
endif ()

# if(MSVC) set_target_properties(Measure PROPERTIES SUFFIX ".pyd") set_target_properties(Measure
# PROPERTIES DEBUG_OUTPUT_NAME "Measure_d") set_target_properties(Measure PROPERTIES
# RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Measure) set_target_properties(Measure PROPERTIES
# PREFIX "../") elseif(MINGW) set_target_properties(Measure PROPERTIES SUFFIX ".pyd")
# set_target_properties(Measure PROPERTIES DEBUG_OUTPUT_NAME "Measure_d")
# set_target_properties(Measure PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Measure)
# set_target_properties(Measure PROPERTIES PREFIX "") else(MSVC) set_target_properties(Measure
# PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Measure) set_target_properties(Measure
# PROPERTIES PREFIX "") set_target_properties(Measure PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
# endif(MSVC)

set_bin_dir (Measure Measure /Mod/Measure)
set_python_prefix_suffix (Measure)

install (TARGETS Measure DESTINATION ${CMAKE_INSTALL_LIBDIR})
