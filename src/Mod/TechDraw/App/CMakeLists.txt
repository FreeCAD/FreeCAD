include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(TechDrawLIBS
    Measure
    Part
    Spreadsheet
    PartDesign
)

if (BUILD_IMPORT)
    list(APPEND TechDrawLIBS Import)
endif ()

include_directories(
    SYSTEM
    ${QtConcurrent_INCLUDE_DIRS}
    ${QtCore_INCLUDE_DIR}
    ${QtGui_INCLUDE_DIR}
    ${QtWidgets_INCLUDE_DIRS}
)

list(APPEND TechDrawLIBS
    ${QtConcurrent_LIBRARIES}
    ${QtCore_LIBRARIES}
    ${QtWidgets_LIBRARIES}
)

generate_from_py(DrawHatchPy)
generate_from_py(DrawViewPy)
generate_from_py(DrawPagePy)
generate_from_py(DrawViewPartPy)
generate_from_py(DrawViewSymbolPy)
generate_from_py(DrawTemplatePy)
generate_from_py(DrawParametricTemplatePy)
generate_from_py(DrawSVGTemplatePy)
generate_from_py(DrawViewClipPy)
generate_from_py(DrawViewDimensionPy)
generate_from_py(DrawViewDimExtentPy)
generate_from_py(DrawGeomHatchPy)
generate_from_py(DrawViewCollectionPy)
generate_from_py(DrawProjGroupPy)
generate_from_py(DrawProjGroupItemPy)
generate_from_py(DrawViewAnnotationPy)
generate_from_py(DrawLeaderLinePy)
generate_from_py(DrawRichAnnoPy)
generate_from_py(GeomFormatPy)
generate_from_py(CenterLinePy)
generate_from_py(CosmeticEdgePy)
generate_from_py(CosmeticVertexPy)
generate_from_py(DrawTilePy)
generate_from_py(DrawTileWeldPy)
generate_from_py(DrawWeldSymbolPy)
generate_from_py(CosmeticExtensionPy)
generate_from_py(DrawBrokenViewPy)

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
    DimensionAutoCorrect.cpp
    DimensionAutoCorrect.h
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
    DrawBrokenView.cpp
    DrawBrokenView.h
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
    LineNameEnum.cpp
    LineNameEnum.h
    ArrowPropEnum.cpp
    ArrowPropEnum.h
    BalloonPropEnum.cpp
    BalloonPropEnum.h
    MattingPropEnum.cpp
    MattingPropEnum.h
    Preferences.cpp
    Preferences.h
    Tag.cpp
    Tag.h
    TechDrawExport.cpp
    TechDrawExport.h
    ProjectionAlgos.cpp
    ProjectionAlgos.h
    XMLQuery.cpp
    XMLQuery.h
    LineGenerator.cpp
    LineGenerator.h
    LineFormat.cpp
    LineFormat.h
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
    GeometryMatcher.cpp
    GeometryMatcher.h
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
    DrawBrokenViewPy.xml
    DrawBrokenViewPyImp.cpp
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
target_link_libraries(TechDraw ${TechDrawLIBS} ${TechDraw})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(TechDraw)
endif()

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
