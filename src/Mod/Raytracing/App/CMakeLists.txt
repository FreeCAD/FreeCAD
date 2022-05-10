if(MSVC)
    add_definitions(-DFCAppRaytracing -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})


set(Raytracing_LIBS
    Part
    ${OCC_LIBRARIES}
    ${OCC_DEBUG_LIBRARIES}
    FreeCADApp
)

macro(generate_from_py2 BASE_NAME OUTPUT_FILE)
    file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/src/Tools/PythonToCPP.py TOOL_PATH)
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME} SOURCE_PATH)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}
        COMMAND ${PYTHON_EXECUTABLE} ${TOOL_PATH} ${SOURCE_PATH} ${OUTPUT_FILE}
        MAIN_DEPENDENCY ${BASE_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT Building files out of ${BASE_NAME}.py)
endmacro(generate_from_py2)

generate_from_py2(FreeCADpov FreeCADpov.h)

SET(Raytracing_SRCS
    AppRaytracing.cpp
    AppRaytracingPy.cpp
    FreeCADpov
    PovTools.cpp
    PovTools.h
    LuxTools.cpp
    LuxTools.h
    PreCompiled.cpp
    PreCompiled.h
    RayFeature.cpp
    RayFeature.h
    RayProject.cpp
    RayProject.h
    RaySegment.cpp
    RaySegment.h
    LuxFeature.h
    LuxFeature.cpp
    LuxProject.h
    LuxProject.cpp
)

add_library(Raytracing SHARED ${Raytracing_SRCS})
target_link_libraries(Raytracing ${Raytracing_LIBS})

SET_BIN_DIR(Raytracing Raytracing /Mod/Raytracing)
SET_PYTHON_PREFIX_SUFFIX(Raytracing)

INSTALL(TARGETS Raytracing DESTINATION ${CMAKE_INSTALL_LIBDIR})
