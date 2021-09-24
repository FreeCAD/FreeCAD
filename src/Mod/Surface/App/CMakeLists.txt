if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)

set(Surface_LIBS
    FreeCADApp
    Part
)

SET(Surface_SRCS
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
