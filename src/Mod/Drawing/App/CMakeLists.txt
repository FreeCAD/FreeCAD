include_directories(
    ${CMAKE_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
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

SET(DrawingComplete_SRCS
    ${Drawing_SRCS}
    ${DrawingAlgos_SRCS}
    ${Features_SRCS}
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${DrawingComplete_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Drawing PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(Drawing SHARED ${Drawing_SRCS} ${Features_SRCS} ${DrawingAlgos_SRCS})
target_link_libraries(Drawing ${Drawing_LIBS})

SET_BIN_DIR(Drawing Drawing /Mod/Drawing)
SET_PYTHON_PREFIX_SUFFIX(Drawing)

INSTALL(TARGETS Drawing DESTINATION ${CMAKE_INSTALL_LIBDIR})
