add_library(Part SHARED)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    Part
    SYSTEM
    PUBLIC
    ${OCC_INCLUDE_DIR}
)

set(Part_LIBS
    ${OCC_LIBRARIES}
    ${OCC_DEBUG_LIBRARIES}
    FreeCADApp
    Materials
)

target_link_directories(Part PUBLIC ${OCC_LIBRARY_DIR})

if(FREETYPE_FOUND)
    add_definitions(-DFCUseFreeType)

    include_directories(
        ${FREETYPE_INCLUDE_DIRS}
    )

    set(Part_LIBS
        ${Part_LIBS}
        ${FREETYPE_LIBRARIES}
    )
endif(FREETYPE_FOUND)

generate_from_py(Arc)
generate_from_py(ArcOfConic)
generate_from_py(ArcOfCircle)
generate_from_py(ArcOfParabola)
generate_from_py(BodyBase)
generate_from_py(Conic)
generate_from_py(Circle)
generate_from_py(ArcOfEllipse)
generate_from_py(Ellipse)
generate_from_py(Hyperbola)
generate_from_py(ArcOfHyperbola)
generate_from_py(Parabola)
generate_from_py(OffsetCurve)
generate_from_py(Geometry)
generate_from_py(GeometryExtension)
generate_from_py(GeometryIntExtension)
generate_from_py(GeometryStringExtension)
generate_from_py(GeometryBoolExtension)
generate_from_py(GeometryDoubleExtension)
generate_from_py(GeometryCurve)
generate_from_py(BoundedCurve)
generate_from_py(TrimmedCurve)
generate_from_py(GeometrySurface)
generate_from_py(Line)
generate_from_py(LineSegment)
generate_from_py(Point)
generate_from_py(BezierCurve)
generate_from_py(BSplineCurve)
generate_from_py(Plane)
generate_from_py(Cone)
generate_from_py(Cylinder)
generate_from_py(Sphere)
generate_from_py(Toroid)
generate_from_py(BezierSurface)
generate_from_py(BSplineSurface)
generate_from_py(OffsetSurface)
generate_from_py(PlateSurface)
generate_from_py(RectangularTrimmedSurface)
generate_from_py(SurfaceOfExtrusion)
generate_from_py(SurfaceOfRevolution)
generate_from_py(PartFeature)
generate_from_py(AttachExtension)
generate_from_py(Part2DObject)
generate_from_py(AttachEngine)
generate_from_py(TopoShape)
generate_from_py(TopoShapeCompound)
generate_from_py(TopoShapeCompSolid)
generate_from_py(TopoShapeEdge)
generate_from_py(TopoShapeFace)
generate_from_py(TopoShapeShell)
generate_from_py(TopoShapeSolid)
generate_from_py(TopoShapeVertex)
generate_from_py(TopoShapeWire)
generate_from_py(BRepOffsetAPI_MakePipeShell)
generate_from_py(BRepOffsetAPI_MakeFilling)


# make sure to create the directory at configure time
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/BRepFeat)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ChFi2d)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Geom2d)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/GeomPlate)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/HLRBRep)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ShapeFix)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ShapeUpgrade)

generate_from_py(BRepFeat/MakePrism)

generate_from_py(ChFi2d/ChFi2d_AnaFilletAlgo)
generate_from_py(ChFi2d/ChFi2d_FilletAlgo)
generate_from_py(ChFi2d/ChFi2d_ChamferAPI)
generate_from_py(ChFi2d/ChFi2d_FilletAPI)

generate_from_py(Geom2d/ArcOfCircle2d)
generate_from_py(Geom2d/ArcOfConic2d)
generate_from_py(Geom2d/ArcOfEllipse2d)
generate_from_py(Geom2d/ArcOfHyperbola2d)
generate_from_py(Geom2d/ArcOfParabola2d)
generate_from_py(Geom2d/BezierCurve2d)
generate_from_py(Geom2d/BSplineCurve2d)
generate_from_py(Geom2d/Circle2d)
generate_from_py(Geom2d/Conic2d)
generate_from_py(Geom2d/Ellipse2d)
generate_from_py(Geom2d/Geometry2d)
generate_from_py(Geom2d/Hyperbola2d)
generate_from_py(Geom2d/Curve2d)
generate_from_py(Geom2d/Line2dSegment)
generate_from_py(Geom2d/Line2d)
generate_from_py(Geom2d/OffsetCurve2d)
generate_from_py(Geom2d/Parabola2d)

generate_from_py(GeomPlate/BuildPlateSurface)
generate_from_py(GeomPlate/CurveConstraint)
generate_from_py(GeomPlate/PointConstraint)

generate_from_py(HLRBRep/HLRBRep_Algo)
generate_from_py(HLRBRep/HLRToShape)
generate_from_py(HLRBRep/HLRBRep_PolyAlgo)
generate_from_py(HLRBRep/PolyHLRToShape)

generate_from_py(ShapeFix/ShapeFix_Root)
generate_from_py(ShapeFix/ShapeFix_Edge)
generate_from_py(ShapeFix/ShapeFix_Face)
generate_from_py(ShapeFix/ShapeFix_Shape)
generate_from_py(ShapeFix/ShapeFix_Shell)
generate_from_py(ShapeFix/ShapeFix_Solid)
generate_from_py(ShapeFix/ShapeFix_Wire)
generate_from_py(ShapeFix/ShapeFix_Wireframe)
generate_from_py(ShapeFix/ShapeFix_WireVertex)
generate_from_py(ShapeFix/ShapeFix_EdgeConnect)
generate_from_py(ShapeFix/ShapeFix_FaceConnect)
generate_from_py(ShapeFix/ShapeFix_FixSmallFace)
generate_from_py(ShapeFix/ShapeFix_FixSmallSolid)
generate_from_py(ShapeFix/ShapeFix_FreeBounds)
generate_from_py(ShapeFix/ShapeFix_ShapeTolerance)
generate_from_py(ShapeFix/ShapeFix_SplitCommonVertex)
generate_from_py(ShapeFix/ShapeFix_SplitTool)

generate_from_py(ShapeUpgrade/UnifySameDomain)

SET(Features_SRCS
    FeaturePartBoolean.cpp
    FeaturePartBoolean.h
    FeaturePartBox.cpp
    FeaturePartBox.h
    FeaturePartCircle.cpp
    FeaturePartCircle.h
    FeaturePartCommon.cpp
    FeaturePartCommon.h
    FeaturePartCurveNet.cpp
    FeaturePartCurveNet.h
    FeaturePartCut.cpp
    FeaturePartCut.h
    FeaturePartFuse.cpp
    FeaturePartFuse.h
    FeaturePartImportBrep.cpp
    FeaturePartImportBrep.h
    FeaturePartImportIges.cpp
    FeaturePartImportIges.h
    FeaturePartImportStep.cpp
    FeaturePartImportStep.h
    FeaturePartPolygon.cpp
    FeaturePartPolygon.h
    FeaturePartSection.cpp
    FeaturePartSection.h
    FeaturePartSpline.cpp
    FeaturePartSpline.h
    FeatureProjectOnSurface.cpp
    FeatureProjectOnSurface.h
    FeatureChamfer.cpp
    FeatureChamfer.h
    FeatureCompound.cpp
    FeatureCompound.h
    FeatureExtrusion.cpp
    FeatureExtrusion.h
    FeatureScale.cpp
    FeatureScale.h
    FeatureFace.cpp
    FeatureFace.h
    FeatureFillet.cpp
    FeatureFillet.h
    FeatureMirroring.cpp
    FeatureMirroring.h
    FeatureRevolution.cpp
    FeatureRevolution.h
    FeatureOffset.cpp
    FeatureOffset.h
    PartFeatures.cpp
    PartFeatures.h
    PartFeature.cpp
    PartFeature.h
    PartFeatureReference.cpp
    PartFeatureReference.h
    Part2DObject.cpp
    Part2DObject.h
    PrimitiveFeature.cpp
    PrimitiveFeature.h
    FeatureGeometrySet.h
    FeatureGeometrySet.cpp
    CustomFeature.cpp
    CustomFeature.h
    BodyBase.h
    BodyBase.cpp
    Datums.cpp
    Datums.h
    DatumFeature.cpp
    DatumFeature.h
    AttachExtension.h
    AttachExtension.cpp
    PrismExtension.cpp
    PrismExtension.h
    VectorAdapter.cpp
    VectorAdapter.h
)
SOURCE_GROUP("Features" FILES ${Features_SRCS})

SET(Properties_SRCS
    PropertyTopoShape.cpp
    PropertyTopoShape.h
    PropertyGeometryList.cpp
    PropertyGeometryList.h
    PropertyTopoShapeList.cpp
    PropertyTopoShapeList.h
)
SOURCE_GROUP("Properties" FILES ${Properties_SRCS})

SET(FCBRepAlgoAPI_SRCS
    FCBRepAlgoAPI_BooleanOperation.cpp
    FCBRepAlgoAPI_BooleanOperation.h
    FCBRepAlgoAPI_Common.cpp
    FCBRepAlgoAPI_Common.h
    FCBRepAlgoAPI_Fuse.cpp
    FCBRepAlgoAPI_Fuse.h
    FCBRepAlgoAPI_Cut.cpp
    FCBRepAlgoAPI_Cut.h
    FCBRepAlgoAPI_Section.cpp
    FCBRepAlgoAPI_Section.h
)
SOURCE_GROUP("FCBRepAlgoAPI-wrapper" FILES ${FCBRepAlgoAPI_SRCS})

SET(Python_SRCS
    Arc.pyi
    ArcPyImp.cpp
    ArcOfConic.pyi
    ArcOfConicPyImp.cpp
    ArcOfCircle.pyi
    ArcOfCirclePyImp.cpp
    ArcOfParabola.pyi
    ArcOfParabolaPyImp.cpp
    BodyBase.pyi
    BodyBasePyImp.cpp
    Conic.pyi
    ConicPyImp.cpp
    Circle.pyi
    CirclePyImp.cpp
    ArcOfEllipse.pyi
    ArcOfEllipsePyImp.cpp
    Ellipse.pyi
    EllipsePyImp.cpp
    Hyperbola.pyi
    HyperbolaPyImp.cpp
    ArcOfHyperbola.pyi
    ArcOfHyperbolaPyImp.cpp
    Parabola.pyi
    ParabolaPyImp.cpp
    OffsetCurve.pyi
    OffsetCurvePyImp.cpp
    Geometry.pyi
    GeometryPyImp.cpp
    GeometryExtension.pyi
    GeometryExtensionPyImp.cpp
    GeometryIntExtension.pyi
    GeometryIntExtensionPyImp.cpp
    GeometryStringExtension.pyi
    GeometryStringExtensionPyImp.cpp
    GeometryBoolExtension.pyi
    GeometryBoolExtensionPyImp.cpp
    GeometryDoubleExtension.pyi
    GeometryDoubleExtensionPyImp.cpp
    GeometryCurve.pyi
    GeometryCurvePyImp.cpp
    BoundedCurve.pyi
    BoundedCurvePyImp.cpp
    TrimmedCurve.pyi
    TrimmedCurvePyImp.cpp
    GeometrySurface.pyi
    GeometrySurfacePyImp.cpp
    Line.pyi
    LinePyImp.cpp
    LineSegment.pyi
    LineSegmentPyImp.cpp
    Point.pyi
    PointPyImp.cpp
    BezierCurve.pyi
    BezierCurvePyImp.cpp
    BSplineCurve.pyi
    BSplineCurvePyImp.cpp
    Plane.pyi
    PlanePyImp.cpp
    Cone.pyi
    ConePyImp.cpp
    Cylinder.pyi
    CylinderPyImp.cpp
    Sphere.pyi
    SpherePyImp.cpp
    Toroid.pyi
    ToroidPyImp.cpp
    BezierSurface.pyi
    BezierSurfacePyImp.cpp
    BSplineSurface.pyi
    BSplineSurfacePyImp.cpp
    OffsetSurface.pyi
    OffsetSurfacePyImp.cpp
    PlateSurface.pyi
    PlateSurfacePyImp.cpp
    RectangularTrimmedSurface.pyi
    RectangularTrimmedSurfacePyImp.cpp
    SurfaceOfExtrusion.pyi
    SurfaceOfExtrusionPyImp.cpp
    SurfaceOfRevolution.pyi
    SurfaceOfRevolutionPyImp.cpp
    PartFeature.pyi
    PartFeaturePyImp.cpp
    AttachExtension.pyi
    AttachExtensionPyImp.cpp
    Part2DObject.pyi
    Part2DObjectPyImp.cpp
    AttachEngine.pyi
    AttachEnginePyImp.cpp
    TopoShape.pyi
    TopoShapePyImp.cpp
    TopoShapeCompSolid.pyi
    TopoShapeCompSolidPyImp.cpp
    TopoShapeCompound.pyi
    TopoShapeCompoundPyImp.cpp
    TopoShapeEdge.pyi
    TopoShapeEdgePyImp.cpp
    TopoShapeFace.pyi
    TopoShapeFacePyImp.cpp
    TopoShapeShell.pyi
    TopoShapeShellPyImp.cpp
    TopoShapeSolid.pyi
    TopoShapeSolidPyImp.cpp
    TopoShapeVertex.pyi
    TopoShapeVertexPyImp.cpp
    TopoShapeWire.pyi
    TopoShapeWirePyImp.cpp
    BRepOffsetAPI_MakePipeShell.pyi
    BRepOffsetAPI_MakePipeShellPyImp.cpp
    BRepOffsetAPI_MakeFilling.pyi
    BRepOffsetAPI_MakeFillingPyImp.cpp
    PartPyCXX.cpp
    PartPyCXX.h
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

# BRepFeat wrappers
SET(BRepFeatPy_SRCS
    BRepFeat/MakePrism.pyi
    BRepFeat/MakePrismPyImp.cpp
)
SOURCE_GROUP("BRepFeat" FILES ${BRepFeatPy_SRCS})

# ChFi2d wrappers
SET(ChFi2dPy_SRCS
    ChFi2d/ChFi2d_AnaFilletAlgo.pyi
    ChFi2d/ChFi2d_AnaFilletAlgoPyImp.cpp
    ChFi2d/ChFi2d_FilletAlgo.pyi
    ChFi2d/ChFi2d_FilletAlgoPyImp.cpp
    ChFi2d/ChFi2d_ChamferAPI.pyi
    ChFi2d/ChFi2d_ChamferAPIPyImp.cpp
    ChFi2d/ChFi2d_FilletAPI.pyi
    ChFi2d/ChFi2d_FilletAPIPyImp.cpp
)
SOURCE_GROUP("ChFi2d" FILES ${ChFi2dPy_SRCS})

# Geom2d wrappers
SET(Geom2dPy_SRCS
    Geom2d/ArcOfCircle2d.pyi
    Geom2d/ArcOfCircle2dPyImp.cpp
    Geom2d/ArcOfConic2d.pyi
    Geom2d/ArcOfConic2dPyImp.cpp
    Geom2d/ArcOfEllipse2d.pyi
    Geom2d/ArcOfEllipse2dPyImp.cpp
    Geom2d/ArcOfHyperbola2d.pyi
    Geom2d/ArcOfHyperbola2dPyImp.cpp
    Geom2d/ArcOfParabola2d.pyi
    Geom2d/ArcOfParabola2dPyImp.cpp
    Geom2d/BezierCurve2d.pyi
    Geom2d/BezierCurve2dPyImp.cpp
    Geom2d/BSplineCurve2d.pyi
    Geom2d/BSplineCurve2dPyImp.cpp
    Geom2d/Circle2d.pyi
    Geom2d/Circle2dPyImp.cpp
    Geom2d/Conic2d.pyi
    Geom2d/Conic2dPyImp.cpp
    Geom2d/Ellipse2d.pyi
    Geom2d/Ellipse2dPyImp.cpp
    Geom2d/Geometry2d.pyi
    Geom2d/Geometry2dPyImp.cpp
    Geom2d/Curve2d.pyi
    Geom2d/Curve2dPyImp.cpp
    Geom2d/Hyperbola2d.pyi
    Geom2d/Hyperbola2dPyImp.cpp
    Geom2d/Line2d.pyi
    Geom2d/Line2dPyImp.cpp
    Geom2d/Line2dSegment.pyi
    Geom2d/Line2dSegmentPyImp.cpp
    Geom2d/OffsetCurve2d.pyi
    Geom2d/OffsetCurve2dPyImp.cpp
    Geom2d/Parabola2d.pyi
    Geom2d/Parabola2dPyImp.cpp
)

SOURCE_GROUP("Geom2d" FILES ${Geom2dPy_SRCS})

# GeomPlate wrappers
SET(GeomPlatePy_SRCS
    GeomPlate/BuildPlateSurface.pyi
    GeomPlate/BuildPlateSurfacePyImp.cpp
    GeomPlate/CurveConstraint.pyi
    GeomPlate/CurveConstraintPyImp.cpp
    GeomPlate/PointConstraint.pyi
    GeomPlate/PointConstraintPyImp.cpp
)

SOURCE_GROUP("GeomPlate" FILES ${GeomPlatePy_SRCS})

# HLRBRep wrappers
SET(HLRBRepPy_SRCS
    HLRBRep/HLRBRep_Algo.pyi
    HLRBRep/HLRBRep_AlgoPyImp.cpp
    HLRBRep/HLRToShape.pyi
    HLRBRep/HLRToShapePyImp.cpp

    HLRBRep/HLRBRep_PolyAlgo.pyi
    HLRBRep/HLRBRep_PolyAlgoPyImp.cpp
    HLRBRep/PolyHLRToShape.pyi
    HLRBRep/PolyHLRToShapePyImp.cpp
)
SOURCE_GROUP("HLRBRep" FILES ${HLRBRepPy_SRCS})

# ShapeFix wrappers
SET(ShapeFixPy_SRCS
    ShapeFix/ShapeFix_Root.pyi
    ShapeFix/ShapeFix_RootPyImp.cpp
    ShapeFix/ShapeFix_Edge.pyi
    ShapeFix/ShapeFix_EdgePyImp.cpp
    ShapeFix/ShapeFix_Face.pyi
    ShapeFix/ShapeFix_FacePyImp.cpp
    ShapeFix/ShapeFix_Shape.pyi
    ShapeFix/ShapeFix_ShapePyImp.cpp
    ShapeFix/ShapeFix_Shell.pyi
    ShapeFix/ShapeFix_ShellPyImp.cpp
    ShapeFix/ShapeFix_Solid.pyi
    ShapeFix/ShapeFix_SolidPyImp.cpp
    ShapeFix/ShapeFix_Wire.pyi
    ShapeFix/ShapeFix_WirePyImp.cpp
    ShapeFix/ShapeFix_Wireframe.pyi
    ShapeFix/ShapeFix_WireframePyImp.cpp
    ShapeFix/ShapeFix_WireVertex.pyi
    ShapeFix/ShapeFix_WireVertexPyImp.cpp
    ShapeFix/ShapeFix_EdgeConnect.pyi
    ShapeFix/ShapeFix_EdgeConnectPyImp.cpp
    ShapeFix/ShapeFix_FaceConnect.pyi
    ShapeFix/ShapeFix_FaceConnectPyImp.cpp
    ShapeFix/ShapeFix_FixSmallFace.pyi
    ShapeFix/ShapeFix_FixSmallFacePyImp.cpp
    ShapeFix/ShapeFix_FixSmallSolid.pyi
    ShapeFix/ShapeFix_FixSmallSolidPyImp.cpp
    ShapeFix/ShapeFix_FreeBounds.pyi
    ShapeFix/ShapeFix_FreeBoundsPyImp.cpp
    ShapeFix/ShapeFix_ShapeTolerance.pyi
    ShapeFix/ShapeFix_ShapeTolerancePyImp.cpp
    ShapeFix/ShapeFix_SplitCommonVertex.pyi
    ShapeFix/ShapeFix_SplitCommonVertexPyImp.cpp
    ShapeFix/ShapeFix_SplitTool.pyi
    ShapeFix/ShapeFix_SplitToolPyImp.cpp
)

SOURCE_GROUP("ShapeFix" FILES ${ShapeFixPy_SRCS})

# ShapeUpgrade wrappers
SET(ShapeUpgradePy_SRCS
    ShapeUpgrade/UnifySameDomain.pyi
    ShapeUpgrade/UnifySameDomainPyImp.cpp
)

SOURCE_GROUP("ShapeUpgrade" FILES ${ShapeUpgradePy_SRCS})

SET(Part_SRCS
    ${Features_SRCS}
    ${Properties_SRCS}
    ${Python_SRCS}
    ${BRepFeatPy_SRCS}
    ${ChFi2dPy_SRCS}
    ${Geom2dPy_SRCS}
    ${GeomPlatePy_SRCS}
    ${HLRBRepPy_SRCS}
    ${ShapeFixPy_SRCS}
    ${ShapeUpgradePy_SRCS}
    IGES/ImportExportSettings.cpp
    IGES/ImportExportSettings.h
    OCAF/ImportExportSettings.cpp
    OCAF/ImportExportSettings.h
    STEP/ImportExportSettings.cpp
    STEP/ImportExportSettings.h
    Attacher.cpp
    Attacher.h
    AppPart.cpp
    AppPartPy.cpp
    BRepMesh.cpp
    BRepMesh.h
    BRepOffsetAPI_MakeOffsetFix.cpp
    BRepOffsetAPI_MakeOffsetFix.h
    BSplineCurveBiArcs.cpp
    BSplineCurveBiArcs.h
    CrossSection.cpp
    CrossSection.h
    ExtrusionHelper.cpp
    ExtrusionHelper.h
    FuzzyHelper.cpp
    FuzzyHelper.h
    GeometryExtension.cpp
    GeometryExtension.h
    GeometryDefaultExtension.cpp
    GeometryDefaultExtension.h
    GeometryMigrationExtension.h
    GeometryMigrationExtension.cpp
    Geometry.cpp
    Geometry.h
    Geometry2d.cpp
    Geometry2d.h
    ImportIges.cpp
    ImportIges.h
    ImportStep.cpp
    ImportStep.h
    Interface.cpp
    Interface.h
    PreCompiled.cpp
    PreCompiled.h
    Services.cpp
    Services.h
    TopoShape.cpp
    TopoShape.h
    TopoShapeCache.cpp
    TopoShapeCache.h
    TopoShapeExpansion.cpp
    TopoShapeMapper.h
    TopoShapeMapper.cpp
    TopoShapeOpCode.h
    edgecluster.cpp
    edgecluster.h
    MeasureClient.cpp
    MeasureClient.h
    MeasureInfo.h
    modelRefine.cpp
    modelRefine.h
    Tools.cpp
    Tools.h
    encodeFilename.h
    OCCError.h
    FT2FC.cpp
    FT2FC.h
    FaceMaker.cpp
    FaceMaker.h
    FaceMakerCheese.cpp
    FaceMakerCheese.h
    FaceMakerBullseye.cpp
    FaceMakerBullseye.h
    WireJoiner.cpp
    WireJoiner.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Part_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Part PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

# Suppress some very long Eigen3 warnings of older versions
if(EIGEN3_NO_DEPRECATED_COPY)
    set_source_files_properties(
        GeometryCurvePyImp.cpp
        GeometrySurfacePyImp.cpp
        PROPERTIES COMPILE_FLAGS ${EIGEN3_NO_DEPRECATED_COPY})
endif()

target_sources(Part PRIVATE ${Part_SRCS} ${FCBRepAlgoAPI_SRCS})
target_link_libraries(Part ${Part_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Part)
endif()

SET_BIN_DIR(Part Part /Mod/Part)
SET_PYTHON_PREFIX_SUFFIX(Part)

INSTALL(TARGETS Part DESTINATION ${CMAKE_INSTALL_LIBDIR})
