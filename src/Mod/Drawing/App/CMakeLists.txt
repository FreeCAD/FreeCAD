if(MSVC)
    add_definitions(-DFCAppDrawing -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H -DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
endif(MSVC)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(Drawing_LIBS
    Part
    FreeCADApp
)

SET(Features_SRCS
    FeaturePage.cpp
    FeaturePage.h
    FeatureProjection.cpp
    FeatureProjection.h
    FeatureView.cpp
    FeatureView.h
    FeatureViewPart.cpp
    FeatureViewPart.h
    PageGroup.cpp
    PageGroup.h
)

SET(Drawing_SRCS
    AppDrawing.cpp
    AppDrawingPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)

SET(DrawingAlgos_SRCS
    ProjectionAlgos.cpp
    ProjectionAlgos.h
)

SOURCE_GROUP("Mod" FILES ${Drawing_SRCS})
SOURCE_GROUP("Features" FILES ${Features_SRCS})
SOURCE_GROUP("Algorithms" FILES ${DrawingAlgos_SRCS})

SET(Drawing_Templates
    Templates/A3_Landscape.svg
    Templates/A4_Simple.svg
)

if(MSVC)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" Drawing_CPP_SRCS ${Drawing_SRCS} ${Features_SRCS} ${DrawingAlgos_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER("PreCompiled.h" "PreCompiled.cpp" Drawing_CPP_SRCS)
endif(MSVC)

add_library(Drawing SHARED ${Drawing_SRCS} ${Features_SRCS} ${DrawingAlgos_SRCS})
target_link_libraries(Drawing ${Drawing_LIBS})
fc_copy_script("Mod/Drawing" "Drawing" Init.py)
fc_copy_script("Mod/Drawing" "Drawing" DrawingAlgos.py)
fc_copy_script("Mod/Drawing" "Drawing" ${Drawing_Templates})

if(MSVC)
    set_target_properties(Drawing PROPERTIES SUFFIX ".pyd")
    set_target_properties(Drawing PROPERTIES DEBUG_OUTPUT_NAME "Drawing_d")
    set_target_properties(Drawing PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Drawing)
    set_target_properties(Drawing PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Drawing PROPERTIES SUFFIX ".pyd")
    set_target_properties(Drawing PROPERTIES DEBUG_OUTPUT_NAME "Drawing_d")
    set_target_properties(Drawing PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Drawing)
    set_target_properties(Drawing PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Drawing PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Drawing)
    set_target_properties(Drawing PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS Drawing DESTINATION lib)
