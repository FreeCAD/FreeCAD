if(MSVC)
    add_definitions(-DFCAppPart -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

if(FREETYPE_FOUND)
    add_definitions(-DFCUseFreeType)
endif(FREETYPE_FOUND)

include_directories(
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${QT_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(Part_LIBS 
    ${OCC_LIBRARIES}
    ${OCC_DEBUG_LIBRARIES}
    FreeCADApp
)

if(FREETYPE_FOUND)
    set(Part_LIBS 
        ${Part_LIBS}
        ${FREETYPE_LIBRARIES}
    )
endif(FREETYPE_FOUND)

generate_from_xml(ArcPy)
generate_from_xml(ArcOfCirclePy)
generate_from_xml(CirclePy)
generate_from_xml(EllipsePy)
generate_from_xml(HyperbolaPy)
generate_from_xml(ParabolaPy)
generate_from_xml(OffsetCurvePy)
generate_from_xml(GeometryPy)
generate_from_xml(GeometryCurvePy)
generate_from_xml(GeometrySurfacePy)
generate_from_xml(LinePy)
generate_from_xml(PointPy)
generate_from_xml(BezierCurvePy)
generate_from_xml(BSplineCurvePy)
generate_from_xml(PlanePy)
generate_from_xml(ConePy)
generate_from_xml(CylinderPy)
generate_from_xml(SpherePy)
generate_from_xml(ToroidPy)
generate_from_xml(BezierSurfacePy)
generate_from_xml(BSplineSurfacePy)
generate_from_xml(OffsetSurfacePy)
generate_from_xml(RectangularTrimmedSurfacePy)
generate_from_xml(SurfaceOfExtrusionPy)
generate_from_xml(SurfaceOfRevolutionPy)
generate_from_xml(PartFeaturePy)
generate_from_xml(Part2DObjectPy)
generate_from_xml(OffsetSurfacePy)
generate_from_xml(OffsetSurfacePy)
generate_from_xml(TopoShapePy)
generate_from_xml(TopoShapeCompoundPy)
generate_from_xml(TopoShapeCompSolidPy)
generate_from_xml(TopoShapeEdgePy)
generate_from_xml(TopoShapeFacePy)
generate_from_xml(TopoShapeShellPy)
generate_from_xml(TopoShapeSolidPy)
generate_from_xml(TopoShapeVertexPy)
generate_from_xml(TopoShapeWirePy)
generate_from_xml(BRepOffsetAPI_MakePipeShellPy)

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
    FeatureChamfer.cpp
    FeatureChamfer.h
    FeatureCompound.cpp
    FeatureCompound.h
    FeatureExtrusion.cpp
    FeatureExtrusion.h
    FeatureFillet.cpp
    FeatureFillet.h
    FeatureMirroring.cpp
    FeatureMirroring.h
    FeatureRevolution.cpp
    FeatureRevolution.h
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
)
SOURCE_GROUP("Features" FILES ${Features_SRCS})

SET(Properties_SRCS
    PropertyTopoShape.cpp
    PropertyTopoShape.h
    PropertyGeometryList.cpp
    PropertyGeometryList.h
)
SOURCE_GROUP("Properties" FILES ${Properties_SRCS})

SET(Python_SRCS
    ArcPy.xml
    ArcPyImp.cpp
    ArcOfCirclePy.xml
    ArcOfCirclePyImp.cpp
    CirclePy.xml
    CirclePyImp.cpp
    EllipsePy.xml
    EllipsePyImp.cpp
    HyperbolaPy.xml
    HyperbolaPyImp.cpp
    ParabolaPy.xml
    ParabolaPyImp.cpp
    OffsetCurvePy.xml
    OffsetCurvePyImp.cpp
    GeometryPy.xml
    GeometryPyImp.cpp
    GeometryCurvePy.xml
    GeometryCurvePyImp.cpp
    GeometrySurfacePy.xml
    GeometrySurfacePyImp.cpp
    LinePy.xml
    LinePyImp.cpp
    PointPy.xml
    PointPyImp.cpp
    BezierCurvePy.xml
    BezierCurvePyImp.cpp
    BSplineCurvePy.xml
    BSplineCurvePyImp.cpp
    PlanePy.xml
    PlanePyImp.cpp
    ConePy.xml
    ConePyImp.cpp
    CylinderPy.xml
    CylinderPyImp.cpp
    SpherePy.xml
    SpherePyImp.cpp
    ToroidPy.xml
    ToroidPyImp.cpp
    BezierSurfacePy.xml
    BezierSurfacePyImp.cpp
    BSplineSurfacePy.xml
    BSplineSurfacePyImp.cpp
    OffsetSurfacePy.xml
    OffsetSurfacePyImp.cpp
    RectangularTrimmedSurfacePy.xml
    RectangularTrimmedSurfacePyImp.cpp
    SurfaceOfExtrusionPy.xml
    SurfaceOfExtrusionPyImp.cpp
    SurfaceOfRevolutionPy.xml
    SurfaceOfRevolutionPyImp.cpp
    PartFeaturePy.xml
    PartFeaturePyImp.cpp
    Part2DObjectPy.xml
    Part2DObjectPyImp.cpp
    TopoShapePy.xml
    TopoShapePyImp.cpp
    TopoShapeCompSolidPy.xml
    TopoShapeCompSolidPyImp.cpp
    TopoShapeCompoundPy.xml
    TopoShapeCompoundPyImp.cpp
    TopoShapeEdgePy.xml
    TopoShapeEdgePyImp.cpp
    TopoShapeFacePy.xml
    TopoShapeFacePyImp.cpp
    TopoShapeShellPy.xml
    TopoShapeShellPyImp.cpp
    TopoShapeSolidPy.xml
    TopoShapeSolidPyImp.cpp
    TopoShapeVertexPy.xml
    TopoShapeVertexPyImp.cpp
    TopoShapeWirePy.xml
    TopoShapeWirePyImp.cpp
    BRepOffsetAPI_MakePipeShellPy.xml
    BRepOffsetAPI_MakePipeShellPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Part_SRCS
    ${Features_SRCS}
    ${Properties_SRCS}
    ${Python_SRCS}
    AppPart.cpp
    AppPartPy.cpp
    CrossSection.cpp
    CrossSection.h
    Geometry.cpp
    Geometry.h
    ImportIges.cpp
    ImportIges.h
    ImportStep.cpp
    ImportStep.h
    PreCompiled.cpp
    PreCompiled.h
    ProgressIndicator.cpp
    ProgressIndicator.h
    TopoShape.cpp
    TopoShape.h
    edgecluster.cpp
    edgecluster.h
    modelRefine.cpp
    modelRefine.h
    Tools.h
    FT2FC.cpp
    FT2FC.h
)

SET(Part_Scripts
    Init.py
    TestPartApp.py
    MakeBottle.py
)

add_library(Part SHARED ${Part_SRCS})
target_link_libraries(Part ${Part_LIBS})


fc_target_copy_resource(Part 
    ${CMAKE_SOURCE_DIR}/src/Mod/Part
    ${CMAKE_BINARY_DIR}/Mod/Part
    ${Part_Scripts})

if(MSVC)
    set_target_properties(Part PROPERTIES SUFFIX ".pyd")
    set_target_properties(Part PROPERTIES DEBUG_OUTPUT_NAME "Part_d")
    set_target_properties(Part PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Part)
    set_target_properties(Part PROPERTIES PREFIX "../")
    # Set special compiler flag to convert a SIGSEV into an exception
    # to fix issue #0000215, #0001155, ...
    set_target_properties(Part PROPERTIES COMPILE_FLAGS "/EHa")
elseif(MINGW)
    set_target_properties(Part PROPERTIES SUFFIX ".pyd")
    set_target_properties(Part PROPERTIES DEBUG_OUTPUT_NAME "Part_d")
    set_target_properties(Part PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Part)
    set_target_properties(Part PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Part PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Part)
    set_target_properties(Part PROPERTIES PREFIX "")
    set_target_properties(Part PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Part DESTINATION lib)
