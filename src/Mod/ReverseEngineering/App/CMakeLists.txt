if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(Reen_LIBS
    Part
    Mesh
    FreeCADApp
)

SET(Reen_SRCS
    AppReverseEngineering.cpp
    AppReverseEngineeringPy.cpp
    ApproxSurface.cpp
    ApproxSurface.h
    PreCompiled.cpp
    PreCompiled.h
)

add_library(ReverseEngineering SHARED ${Reen_SRCS})
target_link_libraries(ReverseEngineering ${Reen_LIBS})
fc_copy_script("Mod/ReverseEngineering" "ReverseEngineering" Init.py)

if(MSVC)
    set_target_properties(ReverseEngineering PROPERTIES SUFFIX ".pyd")
    set_target_properties(ReverseEngineering PROPERTIES DEBUG_OUTPUT_NAME "ReverseEngineering_d")
    set_target_properties(ReverseEngineering PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/ReverseEngineering)
    set_target_properties(ReverseEngineering PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(ReverseEngineering PROPERTIES SUFFIX ".pyd")
    set_target_properties(ReverseEngineering PROPERTIES DEBUG_OUTPUT_NAME "ReverseEngineering_d")
    set_target_properties(ReverseEngineering PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/ReverseEngineering)
    set_target_properties(ReverseEngineering PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(ReverseEngineering PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/ReverseEngineering)
    set_target_properties(ReverseEngineering PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS ReverseEngineering DESTINATION lib)
