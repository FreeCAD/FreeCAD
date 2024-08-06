include_directories(${CMAKE_BINARY_DIR} ${Boost_INCLUDE_DIRS} ${OCC_INCLUDE_DIR}
                    ${ZLIB_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS} ${XercesC_INCLUDE_DIRS})
link_directories(${OCC_LIBRARY_DIR})

set(Drawing_LIBS Part Spreadsheet FreeCADApp)

set(Features_SRCS
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
    FeatureViewSpreadsheet.h)

set(Drawing_SRCS AppDrawing.cpp AppDrawingPy.cpp PreCompiled.cpp PreCompiled.h)

set(DrawingAlgos_SRCS DrawingExport.cpp DrawingExport.h ProjectionAlgos.cpp ProjectionAlgos.h)

source_group("Mod" FILES ${Drawing_SRCS})
source_group("Features" FILES ${Features_SRCS})
source_group("Algorithms" FILES ${DrawingAlgos_SRCS})

set(DrawingComplete_SRCS ${Drawing_SRCS} ${DrawingAlgos_SRCS} ${Features_SRCS})

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    get_msvc_precompiled_source("PreCompiled.cpp" PCH_SRCS ${DrawingComplete_SRCS})
    add_msvc_precompiled_header(Drawing PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(Drawing SHARED ${Drawing_SRCS} ${Features_SRCS} ${DrawingAlgos_SRCS})
target_link_libraries(Drawing ${Drawing_LIBS})
if(FREECAD_WARN_ERROR)
    target_compile_warn_error(Drawing)
endif()

set_bin_dir(Drawing Drawing /Mod/Drawing)
set_python_prefix_suffix(Drawing)

install(TARGETS Drawing DESTINATION ${CMAKE_INSTALL_LIBDIR})
