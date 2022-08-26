if(MSVC)
  add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
  add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${OCC_INCLUDE_DIR}
  ${PYTHON_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIR}
  ${XercesC_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
)

link_directories(${OCC_LIBRARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Blending)

generate_from_xml(Blending/BlendPointPy)
generate_from_xml(Blending/BlendCurvePy)

set(Surface_LIBS
  FreeCADApp
  Part
)

# BlendPoint Wrapper
SET(BlendingPy_SRCS
  Blending/BlendPointPy.xml
  Blending/BlendPointPyImp.cpp
  Blending/BlendCurvePy.xml
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

SET(BlendingPy_SRCS
  Blending/BlendPointPy.xml
  Blending/BlendPointPyImp.cpp
  Blending/BlendCurvePy.xml
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
  PreCompiled.cpp
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
)

link_directories(${OCC_LIBRARY_DIR})
add_library(Surface SHARED ${Surface_SRCS})
target_link_libraries(Surface ${Surface_LIBS})

SET_BIN_DIR(Surface Surface /Mod/Surface)
SET_PYTHON_PREFIX_SUFFIX(Surface)

install(TARGETS Surface DESTINATION ${CMAKE_INSTALL_LIBDIR})
