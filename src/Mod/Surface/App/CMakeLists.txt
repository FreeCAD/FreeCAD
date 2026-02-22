# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Blending)

generate_from_py(Blending/BlendPoint)
generate_from_py(Blending/BlendCurve)

set(Surface_LIBS
  FreeCADApp
  Part
)

# BlendPoint Wrapper
SET(BlendingPy_SRCS
  Blending/BlendPoint.pyi
  Blending/BlendPointPyImp.cpp
  Blending/BlendCurve.pyi
  Blending/BlendCurvePyImp.cpp
)
SOURCE_GROUP("Blending" FILES ${BlendingPy_SRCS})

SET(Blending_SRCS
  Blending/FeatureBlendCurve.cpp
  Blending/FeatureBlendCurve.h
  Blending/BlendPoint.cpp
  Blending/BlendPoint.h
  Blending/BlendCurve.cpp
  Blending/BlendCurve.h
)

SET(Surface_SRCS
  ${Blending_SRCS}
  ${BlendingPy_SRCS}
  AppSurface.cpp
  PreCompiled.h
  FeatureExtend.cpp
  FeatureExtend.h
  FeatureGeomFillSurface.cpp
  FeatureGeomFillSurface.h
  FeatureFilling.cpp
  FeatureFilling.h
  FeatureSections.cpp
  FeatureSections.h
  FeatureSewing.cpp
  FeatureSewing.h
  FeatureCut.cpp
  FeatureCut.h
  Measure.cpp
  Measure.h
)

add_library(Surface SHARED ${Surface_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Surface PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    Surface
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    Surface
    SYSTEM
    PUBLIC
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_directories(Surface PUBLIC ${OCC_LIBRARY_DIR})
target_link_libraries(Surface ${Surface_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Surface)
endif()

SET_BIN_DIR(Surface Surface /Mod/Surface)
SET_PYTHON_PREFIX_SUFFIX(Surface)

install(TARGETS Surface DESTINATION ${CMAKE_INSTALL_LIBDIR})
