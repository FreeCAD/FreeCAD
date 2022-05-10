if(MSVC)
    add_definitions(-DFCAppDrawing -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H -DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(Drawing_LIBS
    Part
    Spreadsheet
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
    FeatureViewAnnotation.cpp
    FeatureViewAnnotation.h
    FeatureViewSymbol.cpp
    FeatureViewSymbol.h
    FeatureClip.cpp
    FeatureClip.h
    PageGroup.cpp
    PageGroup.h
    FeatureViewSpreadsheet.cpp
    FeatureViewSpreadsheet.h
)

SET(Drawing_SRCS
    AppDrawing.cpp
    AppDrawingPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)

SET(DrawingAlgos_SRCS
    DrawingExport.cpp
    DrawingExport.h
    ProjectionAlgos.cpp
    ProjectionAlgos.h
)

SOURCE_GROUP("Mod" FILES ${Drawing_SRCS})
SOURCE_GROUP("Features" FILES ${Features_SRCS})
SOURCE_GROUP("Algorithms" FILES ${DrawingAlgos_SRCS})

add_library(Drawing SHARED ${Drawing_SRCS} ${Features_SRCS} ${DrawingAlgos_SRCS})
target_link_libraries(Drawing ${Drawing_LIBS})

SET_BIN_DIR(Drawing Drawing /Mod/Drawing)
SET_PYTHON_PREFIX_SUFFIX(Drawing)

INSTALL(TARGETS Drawing DESTINATION ${CMAKE_INSTALL_LIBDIR})
