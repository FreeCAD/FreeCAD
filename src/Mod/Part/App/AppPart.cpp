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

#include "OCCError.h"
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
#include "ArcOfEllipsePy.h"
#include "ArcOfParabolaPy.h"
#include "ArcOfHyperbolaPy.h"
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
using namespace Part;
PyObject* Part::PartExceptionOCCError;
PyObject* Part::PartExceptionOCCDomainError;
PyObject* Part::PartExceptionOCCRangeError;
PyObject* Part::PartExceptionOCCConstructionError;
PyObject* Part::PartExceptionOCCDimensionError;

PyDoc_STRVAR(module_part_doc,
"This is a module working with shapes.");

extern "C" {
void PartExport initPart()
{
    std::stringstream str;
    str << OCC_VERSION_MAJOR << "." << OCC_VERSION_MINOR << "." << OCC_VERSION_MAINTENANCE;
#ifdef OCC_VERSION_DEVELOPMENT
    str << "." OCC_VERSION_DEVELOPMENT;
#endif
    App::Application::Config()["OCC_VERSION"] = str.str();

    Base::Console().Log("Module: Part\n");

    // This is highly experimental and we should keep an eye on it
    // if we have mysterious crashes
    // The argument must be 'Standard_False' to avoid FPE caused by
    // Python's cmath module.
#if !defined(_DEBUG)
    OSD::SetSignal(Standard_False);
#endif

    PyObject* partModule = Py_InitModule3("Part", Part_methods, module_part_doc);   /* mod name, table ptr */
    Base::Console().Log("Loading Part module... done\n");
    PyObject* OCCError = 0;
    if (PyObject_IsSubclass(Base::BaseExceptionFreeCADError, 
                PyExc_RuntimeError)) {
        OCCError = PyErr_NewException("Part.OCCError", 
            Base::BaseExceptionFreeCADError, NULL);
    }
    else {
        Base::Console().Error("Can not inherit Part.OCCError form BaseFreeCADError.\n");
        OCCError = PyErr_NewException("Part.OCCError", 
            PyExc_RuntimeError, NULL);
    }
    Py_INCREF(OCCError);
    PyModule_AddObject(partModule, "OCCError", OCCError);
    PartExceptionOCCError = OCCError; //set global variable ;(
    PartExceptionOCCDomainError = PyErr_NewException("Part.OCCDomainError",
            PartExceptionOCCError, NULL);
    Py_INCREF(PartExceptionOCCDomainError);
    PyModule_AddObject(partModule, "OCCDomainError",
            PartExceptionOCCDomainError);
    PartExceptionOCCRangeError = PyErr_NewException("Part.OCCRangeError",
            PartExceptionOCCDomainError, NULL);
    Py_INCREF(PartExceptionOCCRangeError);
    PyModule_AddObject(partModule, "OCCRangeError", PartExceptionOCCRangeError);
    PartExceptionOCCConstructionError = PyErr_NewException(
            "Part.OCCConstructionError", PartExceptionOCCDomainError, NULL);
    Py_INCREF(PartExceptionOCCConstructionError);
    PyModule_AddObject(partModule, "OCCConstructionError",
            PartExceptionOCCConstructionError);
    PartExceptionOCCDimensionError = PyErr_NewException(
            "Part.OCCDimensionError", PartExceptionOCCDomainError, NULL);
    Py_INCREF(PartExceptionOCCConstructionError);
    PyModule_AddObject(partModule, "OCCDimensionError",
            PartExceptionOCCDimensionError);

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
    Part::GeomArcOfEllipse        ::init();
    Part::GeomArcOfParabola       ::init();
    Part::GeomArcOfHyperbola      ::init();
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
    // set the user-defined settings
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part");

    // General
    Base::Reference<ParameterGrp> hGenGrp = hGrp->GetGroup("General");
    // http://www.opencascade.org/org/forum/thread_20801/
    // read.surfacecurve.mode:
    // A preference for the computation of curves in an entity which has both 2D and 3D representation.
    // Each TopoDS_Edge in TopoDS_Face must have a 3D and 2D curve that references the surface.
    // If both 2D and 3D representation of the entity are present, the computation of these curves depends on
    // the following values of parameter:
    // 0: "Default" - no preference, both curves are taken
    // 3: "3DUse_Preferred" - 3D curves are used to rebuild 2D ones
    // Additional modes for IGES
    //  2: "2DUse_Preferred" - the 2D is used to rebuild the 3D in case of their inconsistency
    // -2: "2DUse_Forced" - the 2D is always used to rebuild the 3D (even if 2D is present in the file)
    // -3: "3DUse_Forced" - the 3D is always used to rebuild the 2D (even if 2D is present in the file)
    int readsurfacecurve = hGenGrp->GetInt("ReadSurfaceCurveMode", 0);
    Interface_Static::SetIVal("read.surfacecurve.mode", readsurfacecurve);

    // write.surfacecurve.mode (STEP-only):
    // This parameter indicates whether parametric curves (curves in parametric space of surface) should be
    // written into the STEP file. This parameter can be set to Off in order to minimize the size of the resulting
    // STEP file.
    // Off (0) : writes STEP files without pcurves. This mode decreases the size of the resulting file.
    // On (1) : (default) writes pcurves to STEP file
    int writesurfacecurve = hGenGrp->GetInt("WriteSurfaceCurveMode", 1);
    Interface_Static::SetIVal("write.surfacecurve.mode", writesurfacecurve);

    //IGES handling
    Base::Reference<ParameterGrp> hIgesGrp = hGrp->GetGroup("IGES");
    int value = Interface_Static::IVal("write.iges.brep.mode");
    bool brep = hIgesGrp->GetBool("BrepMode", value > 0);
    Interface_Static::SetIVal("write.iges.brep.mode",brep ? 1 : 0);
    Interface_Static::SetCVal("write.iges.header.company", hIgesGrp->GetASCII("Company").c_str());
    Interface_Static::SetCVal("write.iges.header.author", hIgesGrp->GetASCII("Author").c_str());
  //Interface_Static::SetCVal("write.iges.header.product", hIgesGrp->GetASCII("Product").c_str());

    int unitIges = hIgesGrp->GetInt("Unit", 0);
    switch (unitIges) {
        case 1:
            Interface_Static::SetCVal("write.iges.unit","M");
            break;
        case 2:
            Interface_Static::SetCVal("write.iges.unit","IN");
            break;
        default:
            Interface_Static::SetCVal("write.iges.unit","MM");
            break;
    }

    //STEP handling
    Base::Reference<ParameterGrp> hStepGrp = hGrp->GetGroup("STEP");
    int unitStep = hStepGrp->GetInt("Unit", 0);
    switch (unitStep) {
        case 1:
            Interface_Static::SetCVal("write.step.unit","M");
            break;
        case 2:
            Interface_Static::SetCVal("write.step.unit","IN");
            break;
        default:
            Interface_Static::SetCVal("write.step.unit","MM");
            break;
    }

    std::string ap = hStepGrp->GetASCII("Scheme", Interface_Static::CVal("write.step.schema"));
    Interface_Static::SetCVal("write.step.schema", ap.c_str());
}

} // extern "C"
