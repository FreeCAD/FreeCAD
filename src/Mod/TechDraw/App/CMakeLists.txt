include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${QT_QTCORE_INCLUDE_DIR}
)

set(TechDrawLIBS
    Measure
    Part
    Spreadsheet
    Import
    PartDesign
)

if(FREECAD_QT_MAJOR_VERSION EQUAL 5)
    include_directories(
        ${QtXmlPatterns_INCLUDE_DIRS}
    )
endif()

include_directories(
    ${QtConcurrent_INCLUDE_DIRS}
)
set(QtXmlPatternsLib ${QtXmlPatterns_LIBRARIES})
list(APPEND TechDrawLIBS
    ${QtConcurrent_LIBRARIES}
)

link_directories(${OCC_LIBRARY_DIR})

generate_from_xml(DrawPagePy)
generate_from_xml(DrawViewPy)
generate_from_xml(DrawViewPartPy)
generate_from_xml(DrawViewSymbolPy)
generate_from_xml(DrawTemplatePy)
generate_from_xml(DrawParametricTemplatePy)
generate_from_xml(DrawSVGTemplatePy)
generate_from_xml(DrawViewSymbolPy)
generate_from_xml(DrawViewClipPy)
generate_from_xml(DrawViewDimensionPy)
generate_from_xml(DrawViewDimExtentPy)
generate_from_xml(DrawHatchPy)
generate_from_xml(DrawGeomHatchPy)
generate_from_xml(DrawViewCollectionPy)
generate_from_xml(DrawProjGroupPy)
generate_from_xml(DrawProjGroupItemPy)
generate_from_xml(DrawViewAnnotationPy)
generate_from_xml(DrawLeaderLinePy)
generate_from_xml(DrawRichAnnoPy)
generate_from_xml(GeomFormatPy)
generate_from_xml(CenterLinePy)
generate_from_xml(CosmeticEdgePy)
generate_from_xml(CosmeticVertexPy)
generate_from_xml(DrawTilePy)
generate_from_xml(DrawTileWeldPy)
generate_from_xml(DrawWeldSymbolPy)
generate_from_xml(CosmeticExtensionPy)

SET(Draw_SRCS
    DrawPage.cpp
    DrawPage.h
    DrawComplexSection.cpp
    DrawComplexSection.h
    DrawView.cpp
    DrawView.h
    DrawViewPart.cpp
    DrawViewPart.h
    DrawViewAnnotation.cpp
    DrawViewAnnotation.h
    DrawViewSymbol.cpp
    DrawViewSymbol.h
    DrawViewSpreadsheet.cpp
    DrawViewSpreadsheet.h
    DrawViewClip.cpp
    DrawViewClip.h
    DrawProjGroup.cpp
    DrawProjGroup.h
    DrawProjGroupItem.cpp
    DrawProjGroupItem.h
    DrawParametricTemplate.cpp
    DrawParametricTemplate.h
    DrawSVGTemplate.cpp
    DrawSVGTemplate.h
    DrawTemplate.cpp
    DrawTemplate.h
    DrawViewCollection.cpp
    DrawViewCollection.h
    DrawViewDimension.cpp
    DrawViewDimension.h
    DrawViewDimExtent.cpp
    DrawViewDimExtent.h
    LandmarkDimension.cpp
    LandmarkDimension.h
    DimensionGeometry.cpp
    DimensionGeometry.h
    DimensionReferences.cpp
    DimensionReferences.h
    DimensionFormatter.cpp
    DimensionFormatter.h
    GeometryMatcher.cpp
    GeometryMatcher.h
    DrawViewBalloon.cpp
    DrawViewBalloon.h
    DrawViewSection.cpp
    DrawViewSection.h
    DrawHatch.cpp
    DrawHatch.h
    DrawGeomHatch.cpp
    DrawGeomHatch.h
    DrawViewDraft.cpp
    DrawViewDraft.h
    DrawViewArch.cpp
    DrawViewArch.h
    DrawViewMulti.cpp
    DrawViewMulti.h
    DrawViewImage.cpp
    DrawViewImage.h
    DrawViewDetail.cpp
    DrawViewDetail.h
    DrawLeaderLine.cpp
    DrawLeaderLine.h
    DrawRichAnno.cpp
    DrawRichAnno.h
    DrawTile.cpp
    DrawTile.h
    DrawTileWeld.cpp
    DrawTileWeld.h
    DrawWeldSymbol.cpp
    DrawWeldSymbol.h
    FeatureProjection.cpp
    FeatureProjection.h
    )

SET(TechDraw_SRCS
    AppTechDraw.cpp
    AppTechDrawPy.cpp
    DrawUtil.cpp
    DrawUtil.h
    ShapeExtractor.cpp
    ShapeExtractor.h
    DrawDimHelper.cpp
    DrawDimHelper.h
    HatchLine.cpp
    HatchLine.h
    PreCompiled.cpp
    PreCompiled.h
    EdgeWalker.cpp
    EdgeWalker.h
    DrawProjectSplit.cpp
    DrawProjectSplit.h
    LineGroup.cpp
    LineGroup.h
    ArrowPropEnum.cpp
    ArrowPropEnum.h
    Preferences.cpp
    Preferences.h
    TechDrawExport.cpp
    TechDrawExport.h
    ProjectionAlgos.cpp
    ProjectionAlgos.h
    XMLQuery.cpp
    XMLQuery.h
)

SET(Geometry_SRCS
    Geometry.cpp
    Geometry.h
    GeometryObject.cpp
    GeometryObject.h
    ShapeUtils.cpp
    ShapeUtils.h
    CenterLine.cpp
    CenterLine.h
    Cosmetic.cpp
    Cosmetic.h
    CosmeticVertex.cpp
    CosmeticVertex.h
    PropertyGeomFormatList.cpp
    PropertyGeomFormatList.h
    PropertyCenterLineList.cpp
    PropertyCenterLineList.h
    PropertyCosmeticEdgeList.cpp
    PropertyCosmeticEdgeList.h
    PropertyCosmeticVertexList.cpp
    PropertyCosmeticVertexList.h
    CosmeticExtension.cpp
    CosmeticExtension.h
)

SET(Python_SRCS
    DrawPagePy.xml
    DrawPagePyImp.cpp
    DrawViewPy.xml
    DrawViewPyImp.cpp
    DrawViewPartPy.xml
    DrawViewPartPyImp.cpp
    DrawViewSymbolPy.xml
    DrawViewSymbolPyImp.cpp
    DrawTemplatePy.xml
    DrawTemplatePyImp.cpp
    DrawParametricTemplatePy.xml
    DrawParametricTemplatePyImp.cpp
    DrawSVGTemplatePy.xml
    DrawSVGTemplatePyImp.cpp
    DrawViewClipPy.xml
    DrawViewClipPyImp.cpp
    DrawViewDimensionPy.xml
    DrawViewDimensionPyImp.cpp
    DrawViewDimExtentPy.xml
    DrawViewDimExtentPyImp.cpp
    DrawHatchPy.xml
    DrawHatchPyImp.cpp
    DrawGeomHatchPy.xml
    DrawGeomHatchPyImp.cpp
    DrawViewCollectionPy.xml
    DrawViewCollectionPyImp.cpp
    DrawProjGroupPy.xml
    DrawProjGroupPyImp.cpp
    DrawProjGroupItemPy.xml
    DrawProjGroupItemPyImp.cpp
    DrawViewAnnotationPy.xml
    DrawViewAnnotationPyImp.cpp
    DrawLeaderLinePy.xml
    DrawLeaderLinePyImp.cpp
    DrawRichAnnoPy.xml
    DrawRichAnnoPyImp.cpp
    GeomFormatPy.xml
    GeomFormatPyImp.cpp
    CenterLinePy.xml
    CenterLinePyImp.cpp
    CosmeticEdgePy.xml
    CosmeticEdgePyImp.cpp
    CosmeticVertexPy.xml
    CosmeticVertexPyImp.cpp
    DrawTilePy.xml
    DrawTilePyImp.cpp
    DrawTileWeldPy.xml
    DrawTileWeldPyImp.cpp
    DrawWeldSymbolPy.xml
    DrawWeldSymbolPyImp.cpp
    CosmeticExtensionPy.xml
    CosmeticExtensionPyImp.cpp
    )

SOURCE_GROUP("Mod" FILES ${TechDraw_SRCS})
SOURCE_GROUP("Features" FILES ${Draw_SRCS})
SOURCE_GROUP("Geometry" FILES ${Geometry_SRCS})
SOURCE_GROUP("Python" FILES ${Python_SRCS})

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${TechDraw_SRCS} ${Draw_SRCS} ${TechDrawAlgos_SRCS}
                                                           ${Geometry_SRCS} ${Python_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(TechDraw PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(TechDraw SHARED ${TechDraw_SRCS} ${Draw_SRCS} ${TechDrawAlgos_SRCS}
                           ${Geometry_SRCS} ${Python_SRCS})
target_link_libraries(TechDraw ${TechDrawLIBS};${QtXmlPatternsLib};${TechDraw})

ADD_CUSTOM_COMMAND(TARGET TechDraw
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/src/Mod/TechDraw/Patterns
                   ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw/Patterns
)

ADD_CUSTOM_COMMAND(TARGET TechDraw
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/src/Mod/TechDraw/Templates
                   ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw/Templates
)

ADD_CUSTOM_COMMAND(TARGET TechDraw
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/src/Mod/TechDraw/Symbols
                   ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw/Symbols
)

SET_BIN_DIR(TechDraw TechDraw /Mod/TechDraw)
SET_PYTHON_PREFIX_SUFFIX(TechDraw)

INSTALL(TARGETS TechDraw DESTINATION ${CMAKE_INSTALL_LIBDIR})
