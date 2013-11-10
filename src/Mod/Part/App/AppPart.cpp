/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2002                                                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Python.h>
# include <Interface_Static.hxx>
# include <IGESControl_Controller.hxx>
# include <STEPControl_Controller.hxx>
# include <OSD.hxx>
# include <sstream>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>

#include <App/Application.h>

#include "TopoShape.h"
#include "FeaturePartBox.h"
#include "FeaturePartBoolean.h"
#include "FeaturePartCommon.h"
#include "FeaturePartCut.h"
#include "FeaturePartFuse.h"
#include "FeaturePartSection.h"
#include "FeaturePartImportStep.h"
#include "FeaturePartImportIges.h"
#include "FeaturePartImportBrep.h"
#include "FeaturePartCurveNet.h"
#include "FeaturePartCircle.h"
#include "FeaturePartPolygon.h"
#include "FeaturePartSpline.h"
#include "FeatureGeometrySet.h"
#include "FeatureChamfer.h"
#include "FeatureCompound.h"
#include "FeatureExtrusion.h"
#include "FeatureFillet.h"
#include "FeatureMirroring.h"
#include "FeatureRevolution.h"
#include "PartFeatures.h"
#include "PrimitiveFeature.h"
#include "Part2DObject.h"
#include "CustomFeature.h"
#include "TopoShapePy.h"
#include "TopoShapeVertexPy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeWirePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeCompoundPy.h"
#include "TopoShapeCompSolidPy.h"
#include "TopoShapeShellPy.h"
#include "LinePy.h"
#include "PointPy.h"
#include "CirclePy.h"
#include "EllipsePy.h"
#include "ArcPy.h"
#include "ArcOfCirclePy.h"
#include "BezierCurvePy.h"
#include "BSplineCurvePy.h"
#include "HyperbolaPy.h"
#include "OffsetCurvePy.h"
#include "ParabolaPy.h"
#include "BezierSurfacePy.h"
#include "BSplineSurfacePy.h"
#include "ConePy.h"
#include "CylinderPy.h"
#include "OffsetSurfacePy.h"
#include "PlanePy.h"
#include "RectangularTrimmedSurfacePy.h"
#include "SpherePy.h"
#include "SurfaceOfExtrusionPy.h"
#include "SurfaceOfRevolutionPy.h"
#include "ToroidPy.h"
#include "BRepOffsetAPI_MakePipeShellPy.h"
#include "PartFeaturePy.h"
#include "PropertyGeometryList.h"

extern struct PyMethodDef Part_methods[];

PyDoc_STRVAR(module_part_doc,
"This is a module working with shapes.");

extern "C" {
void PartExport initPart()
{
    std::stringstream str;
    str << OCC_VERSION_MAJOR << "." << OCC_VERSION_MINOR << "." << OCC_VERSION_MAINTENANCE;
    App::Application::Config()["OCC_VERSION"] = str.str();

    // This is highly experimental and we should keep an eye on it
    // if we have mysterious crashes
    // The argument must be 'Standard_False' to avoid FPE caused by
    // Python's cmath module.
//#if defined(FC_OS_LINUX)
    OSD::SetSignal(Standard_False);
//#endif

    PyObject* partModule = Py_InitModule3("Part", Part_methods, module_part_doc);   /* mod name, table ptr */
    Base::Console().Log("Loading Part module... done\n");

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

    Base::Interpreter().addType(&Part::LinePy               ::Type,partModule,"Line");
    Base::Interpreter().addType(&Part::PointPy              ::Type,partModule,"Point");
    Base::Interpreter().addType(&Part::CirclePy             ::Type,partModule,"Circle");
    Base::Interpreter().addType(&Part::EllipsePy            ::Type,partModule,"Ellipse");
    Base::Interpreter().addType(&Part::HyperbolaPy          ::Type,partModule,"Hyperbola");
    Base::Interpreter().addType(&Part::ParabolaPy           ::Type,partModule,"Parabola");
    Base::Interpreter().addType(&Part::ArcPy                ::Type,partModule,"Arc");
    Base::Interpreter().addType(&Part::ArcOfCirclePy        ::Type,partModule,"ArcOfCircle");
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
    Base::Interpreter().addType(&Part::SurfaceOfExtrusionPy ::Type,partModule,"SurfaceOfExtrusion");
    Base::Interpreter().addType(&Part::SurfaceOfRevolutionPy::Type,partModule,"SurfaceOfRevolution");
    Base::Interpreter().addType(&Part::RectangularTrimmedSurfacePy
                                                            ::Type,partModule,"RectangularTrimmedSurface");

    Base::Interpreter().addType(&Part::PartFeaturePy        ::Type,partModule,"Feature");

    PyObject* brepModule = Py_InitModule3("BRepOffsetAPI", 0, "BrepOffsetAPI");
    Py_INCREF(brepModule);
    PyModule_AddObject(partModule, "BRepOffsetAPI", brepModule);
    Base::Interpreter().addType(&Part::BRepOffsetAPI_MakePipeShellPy::Type,brepModule,"MakePipeShell");

    Part::TopoShape             ::init();
    Part::PropertyPartShape     ::init();
    Part::PropertyGeometryList  ::init();
    Part::PropertyShapeHistory  ::init();
    Part::PropertyFilletEdges   ::init();

    Part::Feature               ::init();
    Part::FeatureExt            ::init();
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
    Part::Extrusion             ::init();
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
    Part::RuledSurface          ::init();
    Part::Loft                  ::init();
    Part::Sweep                 ::init();
    Part::Offset                ::init();
    Part::Thickness             ::init();

    // Geometry types
    Part::Geometry                ::init();
    Part::GeomPoint               ::init();
    Part::GeomCurve               ::init();
    Part::GeomBezierCurve         ::init();
    Part::GeomBSplineCurve        ::init();
    Part::GeomCircle              ::init();
    Part::GeomArcOfCircle         ::init();
    Part::GeomEllipse             ::init();
    Part::GeomHyperbola           ::init();
    Part::GeomParabola            ::init();
    Part::GeomLine                ::init();
    Part::GeomLineSegment         ::init();
    Part::GeomOffsetCurve         ::init();
    Part::GeomTrimmedCurve        ::init();
    Part::GeomSurface             ::init();
    Part::GeomBezierSurface       ::init();
    Part::GeomBSplineSurface      ::init();
    Part::GeomCylinder            ::init();
    Part::GeomCone                ::init();
    Part::GeomSphere              ::init();
    Part::GeomToroid              ::init();
    Part::GeomPlane               ::init();
    Part::GeomOffsetSurface       ::init();
    Part::GeomTrimmedSurface      ::init();
    Part::GeomSurfaceOfRevolution ::init();
    Part::GeomSurfaceOfExtrusion  ::init();


    IGESControl_Controller::Init();
    STEPControl_Controller::Init();
    // set the user-defined units
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part");
    int unit = hGrp->GetInt("Unit", 0);
    switch (unit) {
        case 1:
            Interface_Static::SetCVal("write.iges.unit","M");
            Interface_Static::SetCVal("write.step.unit","M");
            break;
        case 2:
            Interface_Static::SetCVal("write.iges.unit","IN");
            Interface_Static::SetCVal("write.step.unit","IN");
            break;
        default:
            Interface_Static::SetCVal("write.iges.unit","MM");
            Interface_Static::SetCVal("write.step.unit","MM");
            break;
    }
}

} // extern "C"
