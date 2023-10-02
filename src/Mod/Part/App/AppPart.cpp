/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Interface_Static.hxx>
# include <IGESControl_Controller.hxx>
# include <STEPControl_Controller.hxx>
# include <Standard_Version.hxx>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/ExceptionFactory.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Base/PrecisionPy.h>

#include "ArcOfCirclePy.h"
#include "ArcOfConicPy.h"
#include "ArcOfEllipsePy.h"
#include "ArcOfHyperbolaPy.h"
#include "ArcOfParabolaPy.h"
#include "ArcPy.h"
#include "Attacher.h"
#include "AttachExtension.h"
#include "AttachEnginePy.h"
#include "BezierCurvePy.h"
#include "BezierSurfacePy.h"
#include "BodyBase.h"
#include "BodyBasePy.h"
#include "BRepOffsetAPI_MakeFillingPy.h"
#include "BRepOffsetAPI_MakePipeShellPy.h"
#include "BSplineCurvePy.h"
#include "BSplineSurfacePy.h"
#include "CirclePy.h"
#include "ConePy.h"
#include "ConicPy.h"
#include "CustomFeature.h"
#include "CylinderPy.h"
#include "DatumFeature.h"
#include "EllipsePy.h"
#include "FaceMaker.h"
#include "FaceMakerBullseye.h"
#include "FaceMakerCheese.h"
#include "FeatureChamfer.h"
#include "FeatureCompound.h"
#include "FeatureExtrusion.h"
#include "FeatureScale.h"
#include "FeatureFace.h"
#include "FeatureFillet.h"
#include "FeatureGeometrySet.h"
#include "FeatureMirroring.h"
#include "FeatureOffset.h"
#include "FeaturePartBoolean.h"
#include "FeaturePartBox.h"
#include "FeaturePartCircle.h"
#include "FeaturePartCommon.h"
#include "FeaturePartCurveNet.h"
#include "FeaturePartCut.h"
#include "FeaturePartFuse.h"
#include "FeaturePartImportBrep.h"
#include "FeaturePartImportIges.h"
#include "FeaturePartImportStep.h"
#include "FeaturePartPolygon.h"
#include "FeaturePartSection.h"
#include "FeaturePartSpline.h"
#include "FeatureRevolution.h"
#include "Geometry.h"
#include "Geometry2d.h"
#include "GeometryBoolExtensionPy.h"
#include "GeometryDefaultExtension.h"
#include "GeometryDoubleExtensionPy.h"
#include "GeometryExtension.h"
#include "GeometryIntExtensionPy.h"
#include "GeometryMigrationExtension.h"
#include "GeometryStringExtensionPy.h"
#include "HyperbolaPy.h"
#include "ImportStep.h"
#include "LinePy.h"
#include "LineSegmentPy.h"
#include "OffsetCurvePy.h"
#include "OffsetSurfacePy.h"
#include "ParabolaPy.h"
#include "Part2DObject.h"
#include "Part2DObjectPy.h"
#include "PartFeaturePy.h"
#include "PartFeatures.h"
#include "PlanePy.h"
#include "PlateSurfacePy.h"
#include "PointPy.h"
#include "PrimitiveFeature.h"
#include "RectangularTrimmedSurfacePy.h"
#include "SpherePy.h"
#include "SurfaceOfExtrusionPy.h"
#include "SurfaceOfRevolutionPy.h"
#include "TopoShapeCompoundPy.h"
#include "TopoShapeCompSolidPy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapePy.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeVertexPy.h"
#include "TopoShapeWirePy.h"
#include "ToroidPy.h"
#include "OCCError.h"
#include "PrismExtension.h"
#include "PropertyGeometryList.h"
#include "PropertyTopoShapeList.h"

#include <BRepFeat/MakePrismPy.h>

#include <ChFi2d/ChFi2d_AnaFilletAlgoPy.h>
#include <ChFi2d/ChFi2d_ChamferAPIPy.h>
#include <ChFi2d/ChFi2d_FilletAlgoPy.h>
#include <ChFi2d/ChFi2d_FilletAPIPy.h>

#include <Geom2d/ArcOfCircle2dPy.h>
#include <Geom2d/ArcOfConic2dPy.h>
#include <Geom2d/ArcOfEllipse2dPy.h>
#include <Geom2d/ArcOfHyperbola2dPy.h>
#include <Geom2d/ArcOfParabola2dPy.h>
#include <Geom2d/BezierCurve2dPy.h>
#include <Geom2d/BSplineCurve2dPy.h>
#include <Geom2d/Circle2dPy.h>
#include <Geom2d/Conic2dPy.h>
#include <Geom2d/Curve2dPy.h>
#include <Geom2d/Ellipse2dPy.h>
#include <Geom2d/Geometry2dPy.h>
#include <Geom2d/Hyperbola2dPy.h>
#include <Geom2d/Line2dPy.h>
#include <Geom2d/Line2dSegmentPy.h>
#include <Geom2d/OffsetCurve2dPy.h>
#include <Geom2d/Parabola2dPy.h>

#include <GeomPlate/BuildPlateSurfacePy.h>
#include <GeomPlate/CurveConstraintPy.h>
#include <GeomPlate/PointConstraintPy.h>

#include <HLRBRep/HLRBRep_AlgoPy.h>
#include <HLRBRep/HLRBRep_PolyAlgoPy.h>
#include <HLRBRep/HLRToShapePy.h>
#include <HLRBRep/PolyHLRToShapePy.h>

#include <ShapeFix/ShapeFix_EdgeConnectPy.h>
#include <ShapeFix/ShapeFix_EdgePy.h>
#include <ShapeFix/ShapeFix_FaceConnectPy.h>
#include <ShapeFix/ShapeFix_FacePy.h>
#include <ShapeFix/ShapeFix_FreeBoundsPy.h>
#include <ShapeFix/ShapeFix_FixSmallFacePy.h>
#include <ShapeFix/ShapeFix_FixSmallSolidPy.h>
#include <ShapeFix/ShapeFix_RootPy.h>
#include <ShapeFix/ShapeFix_ShapePy.h>
#include <ShapeFix/ShapeFix_ShapeTolerancePy.h>
#include <ShapeFix/ShapeFix_ShellPy.h>
#include <ShapeFix/ShapeFix_SolidPy.h>
#include <ShapeFix/ShapeFix_SplitCommonVertexPy.h>
#include <ShapeFix/ShapeFix_SplitToolPy.h>
#include <ShapeFix/ShapeFix_WireframePy.h>
#include <ShapeFix/ShapeFix_WirePy.h>
#include <ShapeFix/ShapeFix_WireVertexPy.h>

#include <ShapeUpgrade/UnifySameDomainPy.h>

#include <OCAF/ImportExportSettings.h>

namespace Part {
extern PyObject* initModule();
}

using namespace Part;

PyObject* Part::PartExceptionOCCError;
PyObject* Part::PartExceptionOCCDomainError;
PyObject* Part::PartExceptionOCCRangeError;
PyObject* Part::PartExceptionOCCConstructionError;
PyObject* Part::PartExceptionOCCDimensionError;

PyMOD_INIT_FUNC(Part)
{
    Base::Console().Log("Module: Part\n");

    // This is highly experimental and we should keep an eye on it
    // if we have mysterious crashes
    // The argument must be 'Standard_False' to avoid FPE caused by
    // Python's cmath module.
    // For Linux use segmentation_fault_handler in Application.cpp
#if !defined(_DEBUG) && !defined(FC_OS_LINUX)
    //OSD::SetSignal(Standard_False);
#endif

    PyObject* partModule = Part::initModule();
    Base::Console().Log("Loading Part module... done\n");

    Py::Object module(partModule);
    module.setAttr("OCC_VERSION", Py::String(OCC_VERSION_STRING_EXT));

    // C++ exceptions
    new Base::ExceptionProducer<Part::NullShapeException>;
    new Base::ExceptionProducer<Part::AttachEngineException>;
    new Base::ExceptionProducer<Part::BooleanException>;

    // Python exceptions
    //
    PyObject* OCCError = nullptr;
    if (PyObject_IsSubclass(Base::PyExc_FC_GeneralError, PyExc_RuntimeError)) {
        OCCError = PyErr_NewException("Part.OCCError", Base::PyExc_FC_GeneralError, nullptr);
    }
    else {
        Base::Console().Error("Can not inherit Part.OCCError form BaseFreeCADError.\n");
        OCCError = PyErr_NewException("Part.OCCError", PyExc_RuntimeError, nullptr);
    }
    Py_INCREF(OCCError);
    PyModule_AddObject(partModule, "OCCError", OCCError);
    PartExceptionOCCError = OCCError; //set global variable ;(

    // domain error
    PartExceptionOCCDomainError = PyErr_NewException("Part.OCCDomainError", PartExceptionOCCError, nullptr);
    Py_INCREF(PartExceptionOCCDomainError);
    PyModule_AddObject(partModule, "OCCDomainError", PartExceptionOCCDomainError);

    // range error
    PartExceptionOCCRangeError = PyErr_NewException("Part.OCCRangeError", PartExceptionOCCDomainError, nullptr);
    Py_INCREF(PartExceptionOCCRangeError);
    PyModule_AddObject(partModule, "OCCRangeError", PartExceptionOCCRangeError);

    // construction error
    PartExceptionOCCConstructionError = PyErr_NewException("Part.OCCConstructionError", PartExceptionOCCDomainError, nullptr);
    Py_INCREF(PartExceptionOCCConstructionError);
    PyModule_AddObject(partModule, "OCCConstructionError", PartExceptionOCCConstructionError);

    // dimension error
    PartExceptionOCCDimensionError = PyErr_NewException("Part.OCCDimensionError", PartExceptionOCCDomainError, nullptr);
    Py_INCREF(PartExceptionOCCConstructionError);
    PyModule_AddObject(partModule, "OCCDimensionError", PartExceptionOCCDimensionError);

    //rename the types properly to pickle and unpickle them
    Part::TopoShapePy         ::Type.tp_name = "Part.Shape";
    Part::TopoShapeVertexPy   ::Type.tp_name = "Part.Vertex";
    Part::TopoShapeWirePy     ::Type.tp_name = "Part.Wire";
    Part::TopoShapeEdgePy     ::Type.tp_name = "Part.Edge";
    Part::TopoShapeSolidPy    ::Type.tp_name = "Part.Solid";
    Part::TopoShapeFacePy     ::Type.tp_name = "Part.Face";
    Part::TopoShapeCompoundPy ::Type.tp_name = "Part.Compound";
    Part::TopoShapeCompSolidPy::Type.tp_name = "Part.CompSolid";
    Part::TopoShapeShellPy    ::Type.tp_name = "Part.Shell";
    // Add Types to module
    Base::Interpreter().addType(&Part::TopoShapePy          ::Type,partModule,"Shape");
    Base::Interpreter().addType(&Part::TopoShapeVertexPy    ::Type,partModule,"Vertex");
    Base::Interpreter().addType(&Part::TopoShapeWirePy      ::Type,partModule,"Wire");
    Base::Interpreter().addType(&Part::TopoShapeEdgePy      ::Type,partModule,"Edge");
    Base::Interpreter().addType(&Part::TopoShapeSolidPy     ::Type,partModule,"Solid");
    Base::Interpreter().addType(&Part::TopoShapeFacePy      ::Type,partModule,"Face");
    Base::Interpreter().addType(&Part::TopoShapeCompoundPy  ::Type,partModule,"Compound");
    Base::Interpreter().addType(&Part::TopoShapeCompSolidPy ::Type,partModule,"CompSolid");
    Base::Interpreter().addType(&Part::TopoShapeShellPy     ::Type,partModule,"Shell");

    // General
    Base::Interpreter().addType(&Part::LinePy               ::Type,partModule,"Line");
    Base::Interpreter().addType(&Part::LineSegmentPy        ::Type,partModule,"LineSegment");
    Base::Interpreter().addType(&Part::PointPy              ::Type,partModule,"Point");
    Base::Interpreter().addType(&Part::ConicPy              ::Type,partModule,"Conic");
    Base::Interpreter().addType(&Part::ArcOfConicPy         ::Type,partModule,"ArcOfConic");
    Base::Interpreter().addType(&Part::CirclePy             ::Type,partModule,"Circle");
    Base::Interpreter().addType(&Part::EllipsePy            ::Type,partModule,"Ellipse");
    Base::Interpreter().addType(&Part::HyperbolaPy          ::Type,partModule,"Hyperbola");
    Base::Interpreter().addType(&Part::ParabolaPy           ::Type,partModule,"Parabola");
    Base::Interpreter().addType(&Part::ArcPy                ::Type,partModule,"Arc");
    Base::Interpreter().addType(&Part::ArcOfCirclePy        ::Type,partModule,"ArcOfCircle");
    Base::Interpreter().addType(&Part::ArcOfEllipsePy       ::Type,partModule,"ArcOfEllipse");
    Base::Interpreter().addType(&Part::ArcOfParabolaPy      ::Type,partModule,"ArcOfParabola");
    Base::Interpreter().addType(&Part::ArcOfHyperbolaPy     ::Type,partModule,"ArcOfHyperbola");
    Base::Interpreter().addType(&Part::BezierCurvePy        ::Type,partModule,"BezierCurve");
    Base::Interpreter().addType(&Part::BSplineCurvePy       ::Type,partModule,"BSplineCurve");
    Base::Interpreter().addType(&Part::OffsetCurvePy        ::Type,partModule,"OffsetCurve");

    Base::Interpreter().addType(&Part::PlanePy              ::Type,partModule,"Plane");
    Base::Interpreter().addType(&Part::CylinderPy           ::Type,partModule,"Cylinder");
    Base::Interpreter().addType(&Part::ConePy               ::Type,partModule,"Cone");
    Base::Interpreter().addType(&Part::SpherePy             ::Type,partModule,"Sphere");
    Base::Interpreter().addType(&Part::ToroidPy             ::Type,partModule,"Toroid");
    Base::Interpreter().addType(&Part::BezierSurfacePy      ::Type,partModule,"BezierSurface");
    Base::Interpreter().addType(&Part::BSplineSurfacePy     ::Type,partModule,"BSplineSurface");
    Base::Interpreter().addType(&Part::OffsetSurfacePy      ::Type,partModule,"OffsetSurface");
    Base::Interpreter().addType(&Part::PlateSurfacePy       ::Type,partModule,"PlateSurface");
    Base::Interpreter().addType(&Part::SurfaceOfExtrusionPy ::Type,partModule,"SurfaceOfExtrusion");
    Base::Interpreter().addType(&Part::SurfaceOfRevolutionPy::Type,partModule,"SurfaceOfRevolution");
    Base::Interpreter().addType(&Part::RectangularTrimmedSurfacePy
                                                            ::Type,partModule,"RectangularTrimmedSurface");

    Base::Interpreter().addType(&Part::PartFeaturePy        ::Type,partModule,"Feature");
    Base::Interpreter().addType(&Part::Part2DObjectPy       ::Type,partModule,"Part2DObject");
    Base::Interpreter().addType(&Part::BodyBasePy           ::Type,partModule,"BodyBase");
    Base::Interpreter().addType(&Attacher::AttachEnginePy   ::Type,partModule,"AttachEngine");

    Base::Interpreter().addType(&Part::GeometryIntExtensionPy ::Type,partModule,"GeometryIntExtension");
    Base::Interpreter().addType(&Part::GeometryStringExtensionPy ::Type,partModule,"GeometryStringExtension");
    Base::Interpreter().addType(&Part::GeometryBoolExtensionPy ::Type,partModule,"GeometryBoolExtension");
    Base::Interpreter().addType(&Part::GeometryDoubleExtensionPy ::Type,partModule,"GeometryDoubleExtension");
    Base::Interpreter().addType(&Base::PrecisionPy ::Type,partModule,"Precision");

    // BRepFeat package
    PyObject* brepfeatModule(module.getAttr("BRepFeat").ptr());
    Base::Interpreter().addType(&Part::MakePrismPy::Type,brepfeatModule,"MakePrism");

    // BRepOffsetAPI package
    PyObject* brepOffsetApiModule(module.getAttr("BRepOffsetAPI").ptr());
    Base::Interpreter().addType(&Part::BRepOffsetAPI_MakePipeShellPy::Type,brepOffsetApiModule,"MakePipeShell");
    Base::Interpreter().addType(&Part::BRepOffsetAPI_MakeFillingPy::Type,brepOffsetApiModule,"MakeFilling");

    // HLRBRep package
    PyObject* hlrfeatModule(module.getAttr("HLRBRep").ptr());
    Base::Interpreter().addType(&Part::HLRBRep_AlgoPy::Type,hlrfeatModule,"Algo");
    Base::Interpreter().addType(&Part::HLRToShapePy::Type,hlrfeatModule,"HLRToShape");
    Base::Interpreter().addType(&Part::HLRBRep_PolyAlgoPy::Type,hlrfeatModule,"PolyAlgo");
    Base::Interpreter().addType(&Part::PolyHLRToShapePy::Type,hlrfeatModule,"PolyHLRToShape");

    // Geom2d package
    PyObject* geom2dModule(module.getAttr("Geom2d").ptr());
    Base::Interpreter().addType(&Part::Geometry2dPy::Type,geom2dModule,"Geometry2d");
    Base::Interpreter().addType(&Part::Curve2dPy::Type,geom2dModule,"Curve2d");
    Base::Interpreter().addType(&Part::Conic2dPy::Type,geom2dModule,"Conic2d");
    Base::Interpreter().addType(&Part::Circle2dPy::Type,geom2dModule,"Circle2d");
    Base::Interpreter().addType(&Part::Ellipse2dPy::Type,geom2dModule,"Ellipse2d");
    Base::Interpreter().addType(&Part::Hyperbola2dPy::Type,geom2dModule,"Hyperbola2d");
    Base::Interpreter().addType(&Part::Parabola2dPy::Type,geom2dModule,"Parabola2d");
    Base::Interpreter().addType(&Part::ArcOfConic2dPy::Type,geom2dModule,"ArcOfConic2d");
    Base::Interpreter().addType(&Part::ArcOfCircle2dPy::Type,geom2dModule,"ArcOfCircle2d");
    Base::Interpreter().addType(&Part::ArcOfEllipse2dPy::Type,geom2dModule,"ArcOfEllipse2d");
    Base::Interpreter().addType(&Part::ArcOfHyperbola2dPy::Type,geom2dModule,"ArcOfHyperbola2d");
    Base::Interpreter().addType(&Part::ArcOfParabola2dPy::Type,geom2dModule,"ArcOfParabola2d");
    Base::Interpreter().addType(&Part::BezierCurve2dPy::Type,geom2dModule,"BezierCurve2d");
    Base::Interpreter().addType(&Part::BSplineCurve2dPy::Type,geom2dModule,"BSplineCurve2d");
    Base::Interpreter().addType(&Part::Line2dSegmentPy::Type,geom2dModule,"Line2dSegment");
    Base::Interpreter().addType(&Part::Line2dPy::Type,geom2dModule,"Line2d");
    Base::Interpreter().addType(&Part::OffsetCurve2dPy::Type,geom2dModule,"OffsetCurve2d");

    // GeomPlate sub-module
    PyObject* geomPlate(module.getAttr("GeomPlate").ptr());
    Base::Interpreter().addType(&Part::BuildPlateSurfacePy::Type, geomPlate, "BuildPlateSurface");
    Base::Interpreter().addType(&Part::CurveConstraintPy::Type, geomPlate, "CurveConstraint");
    Base::Interpreter().addType(&Part::PointConstraintPy::Type, geomPlate, "PointConstraint");

    // ShapeFix sub-module
    PyObject* shapeFix(module.getAttr("ShapeFix").ptr());
    Base::Interpreter().addType(&Part::ShapeFix_RootPy::Type, shapeFix, "Root");
    Base::Interpreter().addType(&Part::ShapeFix_EdgePy::Type, shapeFix, "Edge");
    Base::Interpreter().addType(&Part::ShapeFix_FacePy::Type, shapeFix, "Face");
    Base::Interpreter().addType(&Part::ShapeFix_ShapePy::Type, shapeFix, "Shape");
    Base::Interpreter().addType(&Part::ShapeFix_ShellPy::Type, shapeFix, "Shell");
    Base::Interpreter().addType(&Part::ShapeFix_SolidPy::Type, shapeFix, "Solid");
    Base::Interpreter().addType(&Part::ShapeFix_WirePy::Type, shapeFix, "Wire");
    Base::Interpreter().addType(&Part::ShapeFix_WireframePy::Type, shapeFix, "Wireframe");
    Base::Interpreter().addType(&Part::ShapeFix_WireVertexPy::Type, shapeFix, "WireVertex");
    Base::Interpreter().addType(&Part::ShapeFix_EdgeConnectPy::Type, shapeFix, "EdgeConnect");
    Base::Interpreter().addType(&Part::ShapeFix_FaceConnectPy::Type, shapeFix, "FaceConnect");
    Base::Interpreter().addType(&Part::ShapeFix_FixSmallFacePy::Type, shapeFix, "FixSmallFace");
    Base::Interpreter().addType(&Part::ShapeFix_FixSmallSolidPy::Type, shapeFix, "FixSmallSolid");
    Base::Interpreter().addType(&Part::ShapeFix_FreeBoundsPy::Type, shapeFix, "FreeBounds");
    Base::Interpreter().addType(&Part::ShapeFix_ShapeTolerancePy::Type, shapeFix, "ShapeTolerance");
    Base::Interpreter().addType(&Part::ShapeFix_SplitCommonVertexPy::Type, shapeFix, "SplitCommonVertex");
    Base::Interpreter().addType(&Part::ShapeFix_SplitToolPy::Type, shapeFix, "SplitTool");

    // ShapeUpgrade sub-module
    PyObject* shapeUpgrade(module.getAttr("ShapeUpgrade").ptr());
    Base::Interpreter().addType(&Part::UnifySameDomainPy::Type, shapeUpgrade, "UnifySameDomain");

    // ChFi2d sub-module
    PyObject* chFi2d(module.getAttr("ChFi2d").ptr());
    Base::Interpreter().addType(&Part::ChFi2d_AnaFilletAlgoPy::Type, chFi2d, "AnaFilletAlgo");
    Base::Interpreter().addType(&Part::ChFi2d_FilletAlgoPy::Type, chFi2d, "FilletAlgo");
    Base::Interpreter().addType(&Part::ChFi2d_ChamferAPIPy::Type, chFi2d, "ChamferAPI");
    Base::Interpreter().addType(&Part::ChFi2d_FilletAPIPy::Type, chFi2d, "FilletAPI");

    Part::TopoShape             ::init();
    Part::PropertyPartShape     ::init();
    Part::PropertyGeometryList  ::init();
    Part::PropertyShapeHistory  ::init();
    Part::PropertyFilletEdges   ::init();
    Part::PropertyTopoShapeList ::init();

    Part::FaceMaker             ::init();
    Part::FaceMakerPublic       ::init();
    Part::FaceMakerSimple       ::init();
    Part::FaceMakerCheese       ::init();
    Part::FaceMakerExtrusion    ::init();
    Part::FaceMakerBullseye     ::init();

    Attacher::AttachEngine        ::init();
    Attacher::AttachEngine3D      ::init();
    Attacher::AttachEnginePlane   ::init();
    Attacher::AttachEngineLine    ::init();
    Attacher::AttachEnginePoint   ::init();

    Part::AttachExtension       ::init();
    Part::AttachExtensionPython ::init();

    Part::PrismExtension        ::init();

    Part::Feature               ::init();
    Part::FeatureExt            ::init();
    Part::BodyBase              ::init();
    Part::FeaturePython         ::init();
    Part::FeatureGeometrySet    ::init();
    Part::CustomFeature         ::init();
    Part::CustomFeaturePython   ::init();
    Part::Primitive             ::init();
    Part::Box                   ::init();
    Part::Spline                ::init();
    Part::Boolean               ::init();
    Part::Common                ::init();
    Part::MultiCommon           ::init();
    Part::Cut                   ::init();
    Part::Fuse                  ::init();
    Part::MultiFuse             ::init();
    Part::Section               ::init();
    Part::FilletBase            ::init();
    Part::Fillet                ::init();
    Part::Chamfer               ::init();
    Part::Compound              ::init();
    Part::Compound2             ::init();
    Part::Extrusion             ::init();
    Part::Scale                 ::init();
    Part::Revolution            ::init();
    Part::Mirroring             ::init();
    Part::ImportStep            ::init();
    Part::ImportIges            ::init();
    Part::ImportBrep            ::init();
    Part::CurveNet              ::init();
    Part::Polygon               ::init();
    Part::Circle                ::init();
    Part::Ellipse               ::init();
    Part::Vertex                ::init();
    Part::Line                  ::init();
    Part::Ellipsoid             ::init();
    Part::Plane                 ::init();
    Part::Sphere                ::init();
    Part::Cylinder              ::init();
    Part::Prism                 ::init();
    Part::RegularPolygon        ::init();
    Part::Cone                  ::init();
    Part::Torus                 ::init();
    Part::Helix                 ::init();
    Part::Spiral                ::init();
    Part::Wedge                 ::init();

    Part::Part2DObject          ::init();
    Part::Part2DObjectPython    ::init();
    Part::Face                  ::init();
    Part::RuledSurface          ::init();
    Part::Loft                  ::init();
    Part::Sweep                 ::init();
    Part::Offset                ::init();
    Part::Offset2D              ::init();
    Part::Thickness             ::init();
    Part::Refine                ::init();
    Part::Reverse               ::init();

    // Geometry types
    Part::GeometryExtension       	::init();
    Part::GeometryPersistenceExtension	::init();
    Part::GeometryIntExtension    	::init();
    Part::GeometryStringExtension 	::init();
    Part::GeometryBoolExtension   	::init();
    Part::GeometryDoubleExtension 	::init();
    Part::GeometryMigrationExtension	::init();
    Part::Geometry                	::init();
    Part::GeomPoint               	::init();
    Part::GeomCurve               	::init();
    Part::GeomBoundedCurve        	::init();
    Part::GeomBezierCurve         	::init();
    Part::GeomBSplineCurve        	::init();
    Part::GeomConic               	::init();
    Part::GeomTrimmedCurve        	::init();
    Part::GeomArcOfConic          	::init();
    Part::GeomCircle              	::init();
    Part::GeomArcOfCircle         	::init();
    Part::GeomArcOfEllipse        	::init();
    Part::GeomArcOfParabola       	::init();
    Part::GeomArcOfHyperbola      	::init();
    Part::GeomEllipse             	::init();
    Part::GeomHyperbola           	::init();
    Part::GeomParabola            	::init();
    Part::GeomLine                	::init();
    Part::GeomLineSegment         	::init();
    Part::GeomOffsetCurve         	::init();
    Part::GeomSurface             	::init();
    Part::GeomBezierSurface       	::init();
    Part::GeomBSplineSurface      	::init();
    Part::GeomCylinder            	::init();
    Part::GeomCone                	::init();
    Part::GeomSphere              	::init();
    Part::GeomToroid              	::init();
    Part::GeomPlane               	::init();
    Part::GeomOffsetSurface       	::init();
    Part::GeomPlateSurface        	::init();
    Part::GeomTrimmedSurface      	::init();
    Part::GeomSurfaceOfRevolution 	::init();
    Part::GeomSurfaceOfExtrusion  	::init();
    Part::Datum                   	::init();

    // Geometry2d types
    Part::Geometry2d              ::init();
    Part::Geom2dPoint             ::init();
    Part::Geom2dCurve             ::init();
    Part::Geom2dBezierCurve       ::init();
    Part::Geom2dBSplineCurve      ::init();
    Part::Geom2dConic             ::init();
    Part::Geom2dArcOfConic        ::init();
    Part::Geom2dCircle            ::init();
    Part::Geom2dArcOfCircle       ::init();
    Part::Geom2dEllipse           ::init();
    Part::Geom2dArcOfEllipse      ::init();
    Part::Geom2dHyperbola         ::init();
    Part::Geom2dArcOfHyperbola    ::init();
    Part::Geom2dParabola          ::init();
    Part::Geom2dArcOfParabola     ::init();
    Part::Geom2dLine              ::init();
    Part::Geom2dLineSegment       ::init();
    Part::Geom2dOffsetCurve       ::init();
    Part::Geom2dTrimmedCurve      ::init();

    IGESControl_Controller::Init();
    STEPControl_Controller::Init();

    OCAF::ImportExportSettings::initialize();

    PyMOD_Return(partModule);
}
