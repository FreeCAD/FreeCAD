# SPDX-FileNotice: Part of the FreeCAD project.

set(TechDrawLIBS
    Measure
    Part
    Spreadsheet
    PartDesign
)

if (BUILD_IMPORT)
    list(APPEND TechDrawLIBS Import)
endif ()

list(APPEND TechDrawLIBS
    ${QtConcurrent_LIBRARIES}
    ${QtCore_LIBRARIES}
    ${QtWidgets_LIBRARIES}
)

generate_from_py(DrawHatch)
generate_from_py(DrawView)
generate_from_py(DrawPage)
generate_from_py(DrawViewPart)
generate_from_py(DrawViewSymbol)
generate_from_py(DrawTemplate)
generate_from_py(DrawParametricTemplate)
generate_from_py(DrawSVGTemplate)
generate_from_py(DrawViewClip)
generate_from_py(DrawViewDimension)
generate_from_py(DrawViewDimExtent)
generate_from_py(DrawGeomHatch)
generate_from_py(DrawViewCollection)
generate_from_py(DrawProjGroup)
generate_from_py(DrawProjGroupItem)
generate_from_py(DrawViewAnnotation)
generate_from_py(DrawLeaderLine)
generate_from_py(DrawRichAnno)
generate_from_py(GeomFormat)
generate_from_py(CenterLine)
generate_from_py(CosmeticEdge)
generate_from_py(CosmeticVertex)
generate_from_py(DrawTile)
generate_from_py(DrawTileWeld)
generate_from_py(DrawWeldSymbol)
generate_from_py(CosmeticExtension)
generate_from_py(DrawBrokenView)

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
    DrawPage.pyi
    DrawPagePyImp.cpp
    DrawView.pyi
    DrawViewPyImp.cpp
    DrawViewPart.pyi
    DrawViewPartPyImp.cpp
    DrawViewSymbol.pyi
    DrawViewSymbolPyImp.cpp
    DrawTemplate.pyi
    DrawTemplatePyImp.cpp
    DrawParametricTemplate.pyi
    DrawParametricTemplatePyImp.cpp
    DrawSVGTemplate.pyi
    DrawSVGTemplatePyImp.cpp
    DrawViewClip.pyi
    DrawViewClipPyImp.cpp
    DrawViewDimension.pyi
    DrawViewDimensionPyImp.cpp
    DrawViewDimExtent.pyi
    DrawViewDimExtentPyImp.cpp
    DrawHatch.pyi
    DrawHatchPyImp.cpp
    DrawGeomHatch.pyi
    DrawGeomHatchPyImp.cpp
    DrawViewCollection.pyi
    DrawViewCollectionPyImp.cpp
    DrawProjGroup.pyi
    DrawProjGroupPyImp.cpp
    DrawProjGroupItem.pyi
    DrawProjGroupItemPyImp.cpp
    DrawViewAnnotation.pyi
    DrawViewAnnotationPyImp.cpp
    DrawLeaderLine.pyi
    DrawLeaderLinePyImp.cpp
    DrawRichAnno.pyi
    DrawRichAnnoPyImp.cpp
    GeomFormat.pyi
    GeomFormatPyImp.cpp
    CenterLine.pyi
    CenterLinePyImp.cpp
    CosmeticEdge.pyi
    CosmeticEdgePyImp.cpp
    CosmeticVertex.pyi
    CosmeticVertexPyImp.cpp
    DrawTile.pyi
    DrawTilePyImp.cpp
    DrawTileWeld.pyi
    DrawTileWeldPyImp.cpp
    DrawWeldSymbol.pyi
    DrawWeldSymbolPyImp.cpp
    CosmeticExtension.pyi
    CosmeticExtensionPyImp.cpp
    DrawBrokenView.pyi
    DrawBrokenViewPyImp.cpp
)

SOURCE_GROUP("Mod" FILES ${TechDraw_SRCS})
SOURCE_GROUP("Features" FILES ${Draw_SRCS})
SOURCE_GROUP("Geometry" FILES ${Geometry_SRCS})
SOURCE_GROUP("Python" FILES ${Python_SRCS})

add_library(TechDraw SHARED
    ${TechDraw_SRCS}
    ${Draw_SRCS}
    ${TechDrawAlgos_SRCS}
    ${Geometry_SRCS}
    ${Python_SRCS}
)

if(FREECAD_USE_PCH)
    target_precompile_headers(TechDraw PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    TechDraw
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(
    TechDraw
    SYSTEM
    PUBLIC
    ${QtConcurrent_INCLUDE_DIRS}
    ${QtCore_INCLUDE_DIR}
    ${QtGui_INCLUDE_DIR}
    ${QtWidgets_INCLUDE_DIRS}
)

target_link_libraries(TechDraw ${TechDrawLIBS} ${TechDraw})

if (MSVC)
    target_compile_options(TechDraw PRIVATE /wd4275)
endif()

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
