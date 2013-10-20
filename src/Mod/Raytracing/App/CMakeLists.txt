if(MSVC)
    add_definitions(-DFCAppRaytracing -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
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

SET(Raytracing_Scripts
    Init.py
    RaytracingExample.py
)

SET(Raytracing_Templates
    Templates/LuxClassic.lxs
    Templates/LuxOutdoor.lxs
    Templates/ProjectStd.pov
    Templates/RadiosityNormal.pov
)

add_library(Raytracing SHARED ${Raytracing_SRCS})
target_link_libraries(Raytracing ${Raytracing_LIBS})

fc_target_copy_resource(Raytracing 
    ${CMAKE_SOURCE_DIR}/src/Mod/Raytracing
    ${CMAKE_BINARY_DIR}/Mod/Raytracing
    ${Raytracing_Scripts})

fc_target_copy_resource(Raytracing 
    ${CMAKE_SOURCE_DIR}/src/Mod/Raytracing
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Raytracing
    ${Raytracing_Templates})

if(MSVC)
    set_target_properties(Raytracing PROPERTIES SUFFIX ".pyd")
    set_target_properties(Raytracing PROPERTIES DEBUG_OUTPUT_NAME "Raytracing_d")
    set_target_properties(Raytracing PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Raytracing)
    set_target_properties(Raytracing PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Raytracing PROPERTIES SUFFIX ".pyd")
    set_target_properties(Raytracing PROPERTIES DEBUG_OUTPUT_NAME "Raytracing_d")
    set_target_properties(Raytracing PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Raytracing)
    set_target_properties(Raytracing PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Raytracing PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Raytracing)
    set_target_properties(Raytracing PROPERTIES PREFIX "")
    set_target_properties(Raytracing PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Raytracing DESTINATION lib)
