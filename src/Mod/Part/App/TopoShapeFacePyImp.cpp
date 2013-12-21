/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepTools.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <ShapeAnalysis.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepLProp_SLProps.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <Geom_BezierSurface.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_Plane.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_RectangularTrimmedSurface.hxx>
# include <Geom_SphericalSurface.hxx>
# include <Geom_ToroidalSurface.hxx>
# include <Handle_Geom_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <gp_Pnt2d.hxx>
# include <gp_Pln.hxx>
# include <gp_Cylinder.hxx>
# include <gp_Cone.hxx>
# include <gp_Sphere.hxx>
# include <gp_Torus.hxx>
# include <Standard_Version.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <BRepTopAdaptor_FClass2d.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepLProp_SurfaceTool.hxx>
#include <BRepGProp_Face.hxx>
#include <GeomLProp_SLProps.hxx>

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "TopoShape.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeWirePy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeFacePy.cpp"

#include "BezierSurfacePy.h"
#include "BSplineSurfacePy.h"
#include "PlanePy.h"
#include "CylinderPy.h"
#include "ConePy.h"
#include "SpherePy.h"
#include "OffsetSurfacePy.h"
#include "SurfaceOfRevolutionPy.h"
#include "SurfaceOfExtrusionPy.h"
#include "ToroidPy.h"

using namespace Part;

// returns a string which represent the object e.g. when printed in python
std::string TopoShapeFacePy::representation(void) const
{
    std::stringstream str;
    str << "<Face object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapeFacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of TopoShapeFacePy and the Twin object 
    return new TopoShapeFacePy(new TopoShape);
}

// constructor method
int TopoShapeFacePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject *pW;
    if (PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pW)) {
        try {
            const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(pW)->getTopoShapePtr()->_Shape;
            if (sh.IsNull()) {
                PyErr_SetString(PyExc_Exception, "cannot create face out of empty wire");
                return -1;
            }

            if (sh.ShapeType() == TopAbs_WIRE) {
                BRepBuilderAPI_MakeFace mkFace(TopoDS::Wire(sh));
                if (!mkFace.IsDone()) {
                    PyErr_SetString(PyExc_Exception, "Failed to create face from wire");
                    return -1;
                }
                getTopoShapePtr()->_Shape = mkFace.Face();
                return 0;
            }
            else if (sh.ShapeType() == TopAbs_FACE) {
                getTopoShapePtr()->_Shape = sh;
                return 0;
            }
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return -1;
        }
    }

    PyErr_Clear();
    PyObject *surf, *bound=0;
    if (PyArg_ParseTuple(args, "O!|O!", &(GeometryPy::Type), &surf, &(PyList_Type), &bound)) {
        try {
            Handle_Geom_Surface S = Handle_Geom_Surface::DownCast
                (static_cast<GeometryPy*>(surf)->getGeometryPtr()->handle());
            if (S.IsNull()) {
                PyErr_SetString(PyExc_TypeError, "geometry is not a valid surface");
                return -1;
            }

            BRepBuilderAPI_MakeFace mkFace(S
#if OCC_VERSION_HEX >= 0x060502
              , Precision::Confusion()
#endif
            );
            if (bound) {
                Py::List list(bound);
                for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                    PyObject* item = (*it).ptr();
                    if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                        const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->_Shape;
                        if (sh.ShapeType() == TopAbs_WIRE)
                            mkFace.Add(TopoDS::Wire(sh));
                        else {
                            PyErr_SetString(PyExc_TypeError, "shape is not a wire");
                            return -1;
                        }
                    }
                    else {
                        PyErr_SetString(PyExc_TypeError, "item is not a shape");
                        return -1;
                    }
                }
            }

            getTopoShapePtr()->_Shape = mkFace.Face();
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return -1;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(PyList_Type), &bound)) {
        try {
            std::vector<TopoDS_Wire> wires;
            Py::List list(bound);
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->_Shape;
                    if (sh.ShapeType() == TopAbs_WIRE)
                        wires.push_back(TopoDS::Wire(sh));
                    else
                        Standard_Failure::Raise("shape is not a wire");
                }
                else
                    Standard_Failure::Raise("shape is not a wire");
            }

            if (!wires.empty()) {
                BRepBuilderAPI_MakeFace mkFace(wires.front());
                if (!mkFace.IsDone()) {
                    switch (mkFace.Error()) {
                    case BRepBuilderAPI_NoFace:
                        Standard_Failure::Raise("No face");
                        break;
                    case BRepBuilderAPI_NotPlanar:
                        Standard_Failure::Raise("Not planar");
                        break;
                    case BRepBuilderAPI_CurveProjectionFailed:
                        Standard_Failure::Raise("Curve projection failed");
                        break;
                    case BRepBuilderAPI_ParametersOutOfRange:
                        Standard_Failure::Raise("Parameters out of range");
                        break;
#if OCC_HEX_VERSION < 0x060500
                    case BRepBuilderAPI_SurfaceNotC2:
                        Standard_Failure::Raise("Surface not C2");
                        break;
#endif
                    default:
                        Standard_Failure::Raise("Unknown failure");
                        break;
                    }
                }
                for (std::vector<TopoDS_Wire>::iterator it = wires.begin()+1; it != wires.end(); ++it)
                    mkFace.Add(*it);
                getTopoShapePtr()->_Shape = mkFace.Face();
                return 0;
            }
            else {
                Standard_Failure::Raise("no wires in list");
            }
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return -1;
        }
    }

    PyErr_SetString(PyExc_Exception, "wire or list of wires expected");
    return -1;
}

PyObject* TopoShapeFacePy::makeOffset(PyObject *args)
{
    double dist;
    if (!PyArg_ParseTuple(args, "d",&dist))
        return 0;
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);

    BRepOffsetAPI_MakeOffset mkOffset(f);
    mkOffset.Perform(dist);
    
    return new TopoShapePy(new TopoShape(mkOffset.Shape()));
}

PyObject* TopoShapeFacePy::valueAt(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);

    BRepAdaptor_Surface adapt(f);
    BRepLProp_SLProps prop(adapt,u,v,0,Precision::Confusion());
    const gp_Pnt& V = prop.Value();
    return new Base::VectorPy(new Base::Vector3d(V.X(),V.Y(),V.Z()));
}

PyObject* TopoShapeFacePy::normalAt(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    BRepAdaptor_Surface adapt(f);

    BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
    if (prop.IsNormalDefined()) {
        gp_Pnt pnt; gp_Vec vec;
        // handles the orientation state of the shape
        BRepGProp_Face(f).Normal(u,v,pnt,vec);
        return new Base::VectorPy(new Base::Vector3d(vec.X(),vec.Y(),vec.Z()));
    }
    else {
        PyErr_SetString(PyExc_Exception, "normal not defined");
        return 0;
    }
}

PyObject* TopoShapeFacePy::tangentAt(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    gp_Dir dir;
    Py::Tuple tuple(2);
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    BRepAdaptor_Surface adapt(f);

    BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
    if (prop.IsTangentUDefined()) {
        prop.TangentU(dir);
        tuple.setItem(0, Py::Vector(Base::Vector3d(dir.X(),dir.Y(),dir.Z())));
    }
    else {
        PyErr_SetString(PyExc_Exception, "tangent in u not defined");
        return 0;
    }
    if (prop.IsTangentVDefined()) {
        prop.TangentV(dir);
        tuple.setItem(1, Py::Vector(Base::Vector3d(dir.X(),dir.Y(),dir.Z())));
    }
    else {
        PyErr_SetString(PyExc_Exception, "tangent in v not defined");
        return 0;
    }

    return Py::new_reference_to(tuple);
}

PyObject* TopoShapeFacePy::curvatureAt(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    Py::Tuple tuple(2);
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    BRepAdaptor_Surface adapt(f);

    BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
    if (prop.IsCurvatureDefined()) {
        tuple.setItem(0, Py::Float(prop.MinCurvature()));
        tuple.setItem(1, Py::Float(prop.MaxCurvature()));
    }
    else {
        PyErr_SetString(PyExc_Exception, "curvature not defined");
        return 0;
    }

    return Py::new_reference_to(tuple);
}

PyObject* TopoShapeFacePy::derivative1At(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    Py::Tuple tuple(2);
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    BRepAdaptor_Surface adapt(f);

    try {
        BRepLProp_SLProps prop(adapt,u,v,1,Precision::Confusion());
        const gp_Vec& vecU = prop.D1U();
        tuple.setItem(0, Py::Vector(Base::Vector3d(vecU.X(),vecU.Y(),vecU.Z())));
        const gp_Vec& vecV = prop.D1V();
        tuple.setItem(1, Py::Vector(Base::Vector3d(vecV.X(),vecV.Y(),vecV.Z())));
        return Py::new_reference_to(tuple);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeFacePy::derivative2At(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    Py::Tuple tuple(2);
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    BRepAdaptor_Surface adapt(f);

    try {
        BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
        const gp_Vec& vecU = prop.D2U();
        tuple.setItem(0, Py::Vector(Base::Vector3d(vecU.X(),vecU.Y(),vecU.Z())));
        const gp_Vec& vecV = prop.D2V();
        tuple.setItem(1, Py::Vector(Base::Vector3d(vecV.X(),vecV.Y(),vecV.Z())));
        return Py::new_reference_to(tuple);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeFacePy::isPartOfDomain(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    const TopoDS_Face& face = TopoDS::Face(getTopoShapePtr()->_Shape);

    double tol;
    //double u1, u2, v1, v2, dialen;
    tol = Precision::Confusion();
    try {
        //BRepTools::UVBounds(face, u1, u2, v1, v2);
        //dialen = (u2-u1)*(u2-u1) + (v2-v1)*(v2-v1);
        //dialen = sqrt(dialen)/400.0;
        //tol = std::max<double>(dialen, tol);
        BRepTopAdaptor_FClass2d CL(face,tol);
        TopAbs_State state = CL.Perform(gp_Pnt2d(u,v));
        if (state == TopAbs_ON || state == TopAbs_IN) {
            Py_INCREF(Py_True);
            return Py_True;
        }
        else {
            Py_INCREF(Py_False);
            return Py_False;
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeFacePy::makeHalfSpace(PyObject *args)
{
    PyObject* pPnt;
    if (!PyArg_ParseTuple(args, "O!",&(Base::VectorPy::Type),&pPnt))
        return 0;

    try {
        Base::Vector3d pt = Py::Vector(pPnt,false).toVector();
        BRepPrimAPI_MakeHalfSpace mkHS(TopoDS::Face(this->getTopoShapePtr()->_Shape), gp_Pnt(pt.x,pt.y,pt.z));
        return new TopoShapeSolidPy(new TopoShape(mkHS.Solid()));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeFacePy::validate(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        const TopoDS_Face& face = TopoDS::Face(getTopoShapePtr()->_Shape);
        BRepCheck_Analyzer aChecker(face);
        if (!aChecker.IsValid()) {
            TopoDS_Wire outerwire = ShapeAnalysis::OuterWire(face);
            TopTools_IndexedMapOfShape myMap;
            myMap.Add(outerwire);

            TopExp_Explorer xp(face,TopAbs_WIRE);
            ShapeFix_Wire fix;
            fix.SetFace(face);
            fix.Load(outerwire);
            fix.Perform();
            BRepBuilderAPI_MakeFace mkFace(fix.WireAPIMake());
            while (xp.More()) {
                if (!myMap.Contains(xp.Current())) {
                    fix.Load(TopoDS::Wire(xp.Current()));
                    fix.Perform();
                    mkFace.Add(fix.WireAPIMake());
                }
                xp.Next();
            }

            aChecker.Init(mkFace.Face());
            if (!aChecker.IsValid()) {
                ShapeFix_Shape fix(mkFace.Face());
                fix.SetPrecision(Precision::Confusion());
                fix.SetMaxTolerance(Precision::Confusion());
                fix.SetMaxTolerance(Precision::Confusion());
                fix.Perform();
                fix.FixWireTool()->Perform();
                fix.FixFaceTool()->Perform();
                getTopoShapePtr()->_Shape = fix.Shape();
            }
            else {
                getTopoShapePtr()->_Shape = mkFace.Face();
            }
        }

        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

Py::Object TopoShapeFacePy::getSurface() const
{
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    BRepAdaptor_Surface adapt(f);
    switch(adapt.GetType())
    {
    case GeomAbs_Plane:
        {
            GeomPlane* plane = new GeomPlane();
            Handle_Geom_Plane this_surf = Handle_Geom_Plane::DownCast
                (plane->handle());
            this_surf->SetPln(adapt.Plane());
            return Py::Object(new PlanePy(plane),true);
        }
    case GeomAbs_Cylinder:
        {
            GeomCylinder* cylinder = new GeomCylinder();
            Handle_Geom_CylindricalSurface this_surf = Handle_Geom_CylindricalSurface::DownCast
                (cylinder->handle());
            this_surf->SetCylinder(adapt.Cylinder());
            return Py::Object(new CylinderPy(cylinder),true);
        }
    case GeomAbs_Cone:
        {
            GeomCone* cone = new GeomCone();
            Handle_Geom_ConicalSurface this_surf = Handle_Geom_ConicalSurface::DownCast
                (cone->handle());
            this_surf->SetCone(adapt.Cone());
            return Py::Object(new ConePy(cone),true);
        }
    case GeomAbs_Sphere:
        {
            GeomSphere* sphere = new GeomSphere();
            Handle_Geom_SphericalSurface this_surf = Handle_Geom_SphericalSurface::DownCast
                (sphere->handle());
            this_surf->SetSphere(adapt.Sphere());
            return Py::Object(new SpherePy(sphere),true);
        }
    case GeomAbs_Torus:
        {
            GeomToroid* toroid = new GeomToroid();
            Handle_Geom_ToroidalSurface this_surf = Handle_Geom_ToroidalSurface::DownCast
                (toroid->handle());
            this_surf->SetTorus(adapt.Torus());
            return Py::Object(new ToroidPy(toroid),true);
        }
    case GeomAbs_BezierSurface:
        {
            GeomBezierSurface* surf = new GeomBezierSurface(adapt.Bezier());
            return Py::Object(new BezierSurfacePy(surf),true);
        }
    case GeomAbs_BSplineSurface:
        {
            GeomBSplineSurface* surf = new GeomBSplineSurface(adapt.BSpline());
            return Py::Object(new BSplineSurfacePy(surf),true);
        }
    case GeomAbs_SurfaceOfRevolution:
        {
            Handle_Geom_Surface s = BRep_Tool::Surface(f);
            Handle_Geom_SurfaceOfRevolution rev = Handle_Geom_SurfaceOfRevolution::DownCast(s);
            if (rev.IsNull()) {
                Handle_Geom_RectangularTrimmedSurface rect = Handle_Geom_RectangularTrimmedSurface::DownCast(s);
                rev = Handle_Geom_SurfaceOfRevolution::DownCast(rect->BasisSurface());
            }
            if (!rev.IsNull()) {
                GeomSurfaceOfRevolution* surf = new GeomSurfaceOfRevolution(rev);
                return Py::Object(new SurfaceOfRevolutionPy(surf),true);
            }
            else {
                throw Py::RuntimeError("Failed to convert to surface of revolution");
            }
        }
    case GeomAbs_SurfaceOfExtrusion:
        {
            Handle_Geom_Surface s = BRep_Tool::Surface(f);
            Handle_Geom_SurfaceOfLinearExtrusion ext = Handle_Geom_SurfaceOfLinearExtrusion::DownCast(s);
            if (ext.IsNull()) {
                Handle_Geom_RectangularTrimmedSurface rect = Handle_Geom_RectangularTrimmedSurface::DownCast(s);
                ext = Handle_Geom_SurfaceOfLinearExtrusion::DownCast(rect->BasisSurface());
            }
            if (!ext.IsNull()) {
                GeomSurfaceOfExtrusion* surf = new GeomSurfaceOfExtrusion(ext);
                return Py::Object(new SurfaceOfExtrusionPy(surf),true);
            }
            else {
                throw Py::RuntimeError("Failed to convert to surface of extrusion");
            }
        }
    case GeomAbs_OffsetSurface:
        {
            Handle_Geom_Surface s = BRep_Tool::Surface(f);
            Handle_Geom_OffsetSurface off = Handle_Geom_OffsetSurface::DownCast(s);
            if (off.IsNull()) {
                Handle_Geom_RectangularTrimmedSurface rect = Handle_Geom_RectangularTrimmedSurface::DownCast(s);
                off = Handle_Geom_OffsetSurface::DownCast(rect->BasisSurface());
            }
            if (!off.IsNull()) {
                GeomOffsetSurface* surf = new GeomOffsetSurface(off);
                return Py::Object(new OffsetSurfacePy(surf),true);
            }
            else {
                throw Py::RuntimeError("Failed to convert to offset surface");
            }
        }
    case GeomAbs_OtherSurface:
        break;
    }

    throw Py::TypeError("undefined surface type");
}

PyObject* TopoShapeFacePy::setTolerance(PyObject *args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return 0;
    BRep_Builder aBuilder;
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    aBuilder.UpdateFace(f, tol);
    Py_Return;
}

Py::Float TopoShapeFacePy::getTolerance(void) const
{
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    return Py::Float(BRep_Tool::Tolerance(f));
}

void TopoShapeFacePy::setTolerance(Py::Float tol)
{
    BRep_Builder aBuilder;
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    aBuilder.UpdateFace(f, (double)tol);
}

Py::Tuple TopoShapeFacePy::getParameterRange(void) const
{
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->_Shape);
    BRepAdaptor_Surface adapt(f);
    double u1 = adapt.FirstUParameter();
    double u2 = adapt.LastUParameter();
    double v1 = adapt.FirstVParameter();
    double v2 = adapt.LastVParameter();

    Py::Tuple t(4);
    t.setItem(0, Py::Float(u1));
    t.setItem(1, Py::Float(u2));
    t.setItem(2, Py::Float(v1));
    t.setItem(3, Py::Float(v2));
    return t;
}

// deprecated
Py::Object TopoShapeFacePy::getWire(void) const
{
    try {
        Py::Object sys_out(PySys_GetObject(const_cast<char*>("stdout")));
        Py::Callable write(sys_out.getAttr("write"));
        Py::Tuple arg(1);
        arg.setItem(0, Py::String("Warning: Wire is deprecated, please use OuterWire\n"));
        write.apply(arg);
    }
    catch (const Py::Exception&) {
    }
    return getOuterWire();
}

Py::Object TopoShapeFacePy::getOuterWire(void) const
{
    const TopoDS_Shape& clSh = getTopoShapePtr()->_Shape;
    if (clSh.IsNull())
        throw Py::Exception("Null shape");
    if (clSh.ShapeType() == TopAbs_FACE) {
        TopoDS_Face clFace = (TopoDS_Face&)clSh;
        TopoDS_Wire clWire = ShapeAnalysis::OuterWire(clFace);
        return Py::Object(new TopoShapeWirePy(new TopoShape(clWire)),true);
    }
    else
        throw Py::Exception("Internal error, TopoDS_Shape is not a face!");

    return Py::Object();
}

Py::Object TopoShapeFacePy::getCenterOfMass(void) const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->_Shape, props);
    gp_Pnt c = props.CentreOfMass();
    return Py::Vector(Base::Vector3d(c.X(),c.Y(),c.Z()));
}

PyObject *TopoShapeFacePy::getCustomAttributes(const char* attr) const
{
    return 0;
}

int TopoShapeFacePy::setCustomAttributes(const char* attr, PyObject *obj)
{
    return 0; 
}
