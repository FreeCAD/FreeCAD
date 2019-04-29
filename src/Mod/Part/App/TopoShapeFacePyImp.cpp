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
# include <BRepBuilderAPI_FindPlane.hxx>
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
# include <Geom_Surface.hxx>
# include <Geom2d_Curve.hxx>
# include <Poly_Triangulation.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopoDS_Edge.hxx>
# include <gp_Pnt2d.hxx>
# include <gp_Pln.hxx>
# include <gp_Cylinder.hxx>
# include <gp_Cone.hxx>
# include <gp_Sphere.hxx>
# include <gp_Torus.hxx>
# include <Standard_Version.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_Wire.hxx>
# include <TColgp_Array1OfPnt2d.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <BRepTopAdaptor_FClass2d.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GProp_PrincipalProps.hxx>
#include <BRepLProp_SurfaceTool.hxx>
#include <BRepGProp_Face.hxx>
#include <GeomLProp_SLProps.hxx>
#include <BRep_Tool.hxx>

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "TopoShape.h"
#include "Geometry2d.h"
#include <Mod/Part/App/TopoShapeSolidPy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>
#include <Mod/Part/App/TopoShapeFacePy.h>
#include <Mod/Part/App/TopoShapeFacePy.cpp>
#include <Mod/Part/App/TopoShapeCompoundPy.h>

#include <Mod/Part/App/BezierSurfacePy.h>
#include <Mod/Part/App/BSplineSurfacePy.h>
#include <Mod/Part/App/PlanePy.h>
#include <Mod/Part/App/CylinderPy.h>
#include <Mod/Part/App/ConePy.h>
#include <Mod/Part/App/SpherePy.h>
#include <Mod/Part/App/OffsetSurfacePy.h>
#include <Mod/Part/App/SurfaceOfRevolutionPy.h>
#include <Mod/Part/App/SurfaceOfExtrusionPy.h>
#include <Mod/Part/App/ToroidPy.h>
#include "OCCError.h"
#include "Tools.h"
#include "FaceMaker.h"

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
            const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(pW)->getTopoShapePtr()->getShape();
            if (sh.IsNull()) {
                PyErr_SetString(PartExceptionOCCError, "cannot create face out of empty wire");
                return -1;
            }

            if (sh.ShapeType() == TopAbs_WIRE) {
                BRepBuilderAPI_MakeFace mkFace(TopoDS::Wire(sh));
                if (!mkFace.IsDone()) {
                    PyErr_SetString(PartExceptionOCCError, "Failed to create face from wire");
                    return -1;
                }
                getTopoShapePtr()->setShape(mkFace.Face());
                return 0;
            }
            else if (sh.ShapeType() == TopAbs_FACE) {
                getTopoShapePtr()->setShape(sh);
                return 0;
            }
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
    }

    PyErr_Clear();
    PyObject *face, *wire;
    if (PyArg_ParseTuple(args, "O!O!", &(Part::TopoShapeFacePy::Type), &face,
                                       &(Part::TopoShapeWirePy::Type), &wire)) {
        try {
            const TopoDS_Shape& f = static_cast<Part::TopoShapePy*>(face)->getTopoShapePtr()->getShape();
            if (f.IsNull()) {
                PyErr_SetString(PartExceptionOCCError, "cannot create face out of empty support face");
                return -1;
            }
            const TopoDS_Shape& w = static_cast<Part::TopoShapePy*>(wire)->getTopoShapePtr()->getShape();
            if (w.IsNull()) {
                PyErr_SetString(PartExceptionOCCError, "cannot create face out of empty boundary wire");
                return -1;
            }

            const TopoDS_Face& supportFace = TopoDS::Face(f);
            const TopoDS_Wire& boundaryWire = TopoDS::Wire(w);
            BRepBuilderAPI_MakeFace mkFace(supportFace, boundaryWire);
            if (!mkFace.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, "Failed to create face from wire");
                return -1;
            }
            getTopoShapePtr()->setShape(mkFace.Face());
            return 0;
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
    }

    PyErr_Clear();
    PyObject *surf;
    if (PyArg_ParseTuple(args, "O!O!", &(Part::GeometrySurfacePy::Type), &surf,
                                       &(Part::TopoShapeWirePy::Type), &wire)) {
        try {
            Handle(Geom_Surface) S = Handle(Geom_Surface)::DownCast
                (static_cast<GeometryPy*>(surf)->getGeometryPtr()->handle());
            if (S.IsNull()) {
                PyErr_SetString(PyExc_TypeError, "geometry is not a valid surface");
                return -1;
            }
            const TopoDS_Shape& w = static_cast<Part::TopoShapePy*>(wire)->getTopoShapePtr()->getShape();
            if (w.IsNull()) {
                PyErr_SetString(PartExceptionOCCError, "cannot create face out of empty boundary wire");
                return -1;
            }

            const TopoDS_Wire& boundaryWire = TopoDS::Wire(w);
            BRepBuilderAPI_MakeFace mkFace(S, boundaryWire);
            if (!mkFace.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, "Failed to create face from wire");
                return -1;
            }
            getTopoShapePtr()->setShape(mkFace.Face());
            return 0;
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
    }

    PyErr_Clear();
    PyObject *bound=0;
    if (PyArg_ParseTuple(args, "O!|O!", &(GeometryPy::Type), &surf, &(PyList_Type), &bound)) {
        try {
            Handle(Geom_Surface) S = Handle(Geom_Surface)::DownCast
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
                        const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape();
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

            getTopoShapePtr()->setShape(mkFace.Face());
            return 0;
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
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
                    const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape();
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
#if OCC_VERSION_HEX < 0x060500
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
                getTopoShapePtr()->setShape(mkFace.Face());
                return 0;
            }
            else {
                Standard_Failure::Raise("no wires in list");
            }
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
    }

    char* className = 0;
    PyObject* pcPyShapeOrList = nullptr;
    PyErr_Clear();
    if (PyArg_ParseTuple(args, "Os", &pcPyShapeOrList, &className)) {
        try {
            std::unique_ptr<FaceMaker> fm = Part::FaceMaker::ConstructFromType(className);

            //dump all supplied shapes to facemaker, no matter what type (let facemaker decide).
            if (PySequence_Check(pcPyShapeOrList)){
                Py::Sequence list(pcPyShapeOrList);
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    PyObject* item = (*it).ptr();
                    if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                        const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape();
                        fm->addShape(sh);
                    } else {
                        PyErr_SetString(PyExc_TypeError, "Object is not a shape.");
                        return -1;
                    }
                }
            } else if (PyObject_TypeCheck(pcPyShapeOrList, &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(pcPyShapeOrList)->getTopoShapePtr()->getShape();
                if (sh.IsNull())
                    throw NullShapeException("Shape is null!");
                if (sh.ShapeType() == TopAbs_COMPOUND)
                    fm->useCompound(TopoDS::Compound(sh));
                else
                    fm->addShape(sh);
            } else {
                PyErr_SetString(PyExc_TypeError, "First argument is neither a shape nor list of shapes.");
                return -1;
            }

            fm->Build();

            getTopoShapePtr()->setShape(fm->Face());
            return 0;
        } catch (Base::Exception &e){
            PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
            return -1;
        } catch (Standard_Failure& e){
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
    }

    PyErr_SetString(PartExceptionOCCError,
      "Argument list signature is incorrect.\n\nSupported signatures:\n"
      "(face)\n"
      "(wire)\n"
      "(face, wire)\n"
      "(surface, wire)\n"
      "(list_of_wires)\n"
      "(wire, facemaker_class_name)\n"
      "(list_of_wires, facemaker_class_name)\n"
      "(surface, list_of_wires)\n"
                    );
    return -1;
}

PyObject* TopoShapeFacePy::makeOffset(PyObject *args)
{
    double dist;
    if (!PyArg_ParseTuple(args, "d",&dist))
        return 0;
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    BRepBuilderAPI_FindPlane findPlane(f);
    if (!findPlane.Found()) {
        PyErr_SetString(PartExceptionOCCError, "No planar face");
        return 0;
    }

    BRepOffsetAPI_MakeOffset mkOffset(f);
    mkOffset.Perform(dist);
    
    return new TopoShapePy(new TopoShape(mkOffset.Shape()));
}

PyObject* TopoShapeFacePy::valueAt(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());

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

    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    BRepAdaptor_Surface adapt(f);

    BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
    if (prop.IsNormalDefined()) {
        gp_Pnt pnt; gp_Vec vec;
        // handles the orientation state of the shape
        BRepGProp_Face(f).Normal(u,v,pnt,vec);
        vec.Normalize();
        return new Base::VectorPy(new Base::Vector3d(vec.X(),vec.Y(),vec.Z()));
    }
    else {
        PyErr_SetString(PartExceptionOCCError, "normal not defined");
        return 0;
    }
}

PyObject* TopoShapeFacePy::getUVNodes(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    TopLoc_Location aLoc;
    Handle (Poly_Triangulation) mesh = BRep_Tool::Triangulation(f,aLoc);
    if (mesh.IsNull()) {
        PyErr_SetString(PyExc_RuntimeError, "Face has no triangulation");
        return 0;
    }

    Py::List list;
    if (!mesh->HasUVNodes()) {
        return Py::new_reference_to(list);
    }

    const TColgp_Array1OfPnt2d& aNodesUV = mesh->UVNodes();
    for (int i=aNodesUV.Lower(); i<=aNodesUV.Upper(); i++) {
        gp_Pnt2d pt2d = aNodesUV(i);
        Py::Tuple uv(2);
        uv.setItem(0, Py::Float(pt2d.X()));
        uv.setItem(1, Py::Float(pt2d.Y()));
        list.append(uv);
    }

    return Py::new_reference_to(list);
}

PyObject* TopoShapeFacePy::tangentAt(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    gp_Dir dir;
    Py::Tuple tuple(2);
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    BRepAdaptor_Surface adapt(f);

    BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
    if (prop.IsTangentUDefined()) {
        prop.TangentU(dir);
        tuple.setItem(0, Py::Vector(Base::Vector3d(dir.X(),dir.Y(),dir.Z())));
    }
    else {
        PyErr_SetString(PartExceptionOCCError, "tangent in u not defined");
        return 0;
    }
    if (prop.IsTangentVDefined()) {
        prop.TangentV(dir);
        tuple.setItem(1, Py::Vector(Base::Vector3d(dir.X(),dir.Y(),dir.Z())));
    }
    else {
        PyErr_SetString(PartExceptionOCCError, "tangent in v not defined");
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
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    BRepAdaptor_Surface adapt(f);

    BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
    if (prop.IsCurvatureDefined()) {
        tuple.setItem(0, Py::Float(prop.MinCurvature()));
        tuple.setItem(1, Py::Float(prop.MaxCurvature()));
    }
    else {
        PyErr_SetString(PartExceptionOCCError, "curvature not defined");
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
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    BRepAdaptor_Surface adapt(f);

    try {
        BRepLProp_SLProps prop(adapt,u,v,1,Precision::Confusion());
        const gp_Vec& vecU = prop.D1U();
        tuple.setItem(0, Py::Vector(Base::Vector3d(vecU.X(),vecU.Y(),vecU.Z())));
        const gp_Vec& vecV = prop.D1V();
        tuple.setItem(1, Py::Vector(Base::Vector3d(vecV.X(),vecV.Y(),vecV.Z())));
        return Py::new_reference_to(tuple);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeFacePy::derivative2At(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    Py::Tuple tuple(2);
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    BRepAdaptor_Surface adapt(f);

    try {
        BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
        const gp_Vec& vecU = prop.D2U();
        tuple.setItem(0, Py::Vector(Base::Vector3d(vecU.X(),vecU.Y(),vecU.Z())));
        const gp_Vec& vecV = prop.D2V();
        tuple.setItem(1, Py::Vector(Base::Vector3d(vecV.X(),vecV.Y(),vecV.Z())));
        return Py::new_reference_to(tuple);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeFacePy::isPartOfDomain(PyObject *args)
{
    double u,v;
    if (!PyArg_ParseTuple(args, "dd",&u,&v))
        return 0;

    const TopoDS_Face& face = TopoDS::Face(getTopoShapePtr()->getShape());

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
        return PyBool_FromLong((state == TopAbs_ON || state == TopAbs_IN) ? 1 : 0);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
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
        BRepPrimAPI_MakeHalfSpace mkHS(TopoDS::Face(this->getTopoShapePtr()->getShape()), gp_Pnt(pt.x,pt.y,pt.z));
        return new TopoShapeSolidPy(new TopoShape(mkHS.Solid()));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeFacePy::validate(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        const TopoDS_Face& face = TopoDS::Face(getTopoShapePtr()->getShape());
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
                fix.Perform();
                fix.FixWireTool()->Perform();
                fix.FixFaceTool()->Perform();
                getTopoShapePtr()->setShape(fix.Shape());
            }
            else {
                getTopoShapePtr()->setShape(mkFace.Face());
            }
        }

        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeFacePy::curveOnSurface(PyObject *args)
{
    PyObject* e;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapeEdgePy::Type), &e))
        return 0;

    try {
        TopoDS_Shape shape = static_cast<TopoShapeEdgePy*>(e)->getTopoShapePtr()->getShape();
        if (shape.IsNull()) {
            PyErr_SetString(PyExc_RuntimeError, "invalid shape");
            return 0;
        }

        TopoDS_Edge edge = TopoDS::Edge(shape);
        const TopoDS_Face& face = TopoDS::Face(getTopoShapePtr()->getShape());

        Standard_Real first, last;
        Handle(Geom2d_Curve) curve = BRep_Tool::CurveOnSurface(edge, face, first, last);
        std::unique_ptr<Part::Geom2dCurve> geo2d = getCurve2dFromGeom2d(curve);
        if (!geo2d)
            Py_Return;

        Py::Tuple tuple(3);
        tuple.setItem(0, Py::asObject(geo2d->getPyObject()));
        tuple.setItem(1, Py::Float(first));
        tuple.setItem(2, Py::Float(last));
        return Py::new_reference_to(tuple);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeFacePy::cutHoles(PyObject *args)
{
    PyObject *holes=0;
    if (PyArg_ParseTuple(args, "O!", &(PyList_Type), &holes)) {
        try {
            std::vector<TopoDS_Wire> wires;
            Py::List list(holes);
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape();
                    if (sh.ShapeType() == TopAbs_WIRE)
                        wires.push_back(TopoDS::Wire(sh));
                    else
                        Standard_Failure::Raise("shape is not a wire");
                }
                else
                    Standard_Failure::Raise("argument is not a shape");
            }

            if (!wires.empty()) {
                const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
                BRepBuilderAPI_MakeFace mkFace(f);
                for (std::vector<TopoDS_Wire>::iterator it = wires.begin(); it != wires.end(); ++it)
                    mkFace.Add(*it);
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
#if OCC_VERSION_HEX < 0x060500
                    case BRepBuilderAPI_SurfaceNotC2:
                        Standard_Failure::Raise("Surface not C2");
                        break;
#endif
                    default:
                        Standard_Failure::Raise("Unknown failure");
                        break;
                    }
                }

                getTopoShapePtr()->setShape(mkFace.Face());
                Py_Return;
            }
            else {
                Standard_Failure::Raise("empty wire list");
            }
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return 0;
        }
    }
    PyErr_SetString(PyExc_RuntimeError, "invalid list of wires");
    return 0;
}


Py::Object TopoShapeFacePy::getSurface() const
{
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    BRepAdaptor_Surface adapt(f);
    Base::PyObjectBase* surface = 0;
    switch(adapt.GetType())
    {
    case GeomAbs_Plane:
        {
            GeomPlane* plane = new GeomPlane();
            Handle(Geom_Plane) this_surf = Handle(Geom_Plane)::DownCast
                (plane->handle());
            this_surf->SetPln(adapt.Plane());
            surface = new PlanePy(plane);
            break;
        }
    case GeomAbs_Cylinder:
        {
            GeomCylinder* cylinder = new GeomCylinder();
            Handle(Geom_CylindricalSurface) this_surf = Handle(Geom_CylindricalSurface)::DownCast
                (cylinder->handle());
            this_surf->SetCylinder(adapt.Cylinder());
            surface = new CylinderPy(cylinder);
            break;
        }
    case GeomAbs_Cone:
        {
            GeomCone* cone = new GeomCone();
            Handle(Geom_ConicalSurface) this_surf = Handle(Geom_ConicalSurface)::DownCast
                (cone->handle());
            this_surf->SetCone(adapt.Cone());
            surface = new ConePy(cone);
            break;
        }
    case GeomAbs_Sphere:
        {
            GeomSphere* sphere = new GeomSphere();
            Handle(Geom_SphericalSurface) this_surf = Handle(Geom_SphericalSurface)::DownCast
                (sphere->handle());
            this_surf->SetSphere(adapt.Sphere());
            surface = new SpherePy(sphere);
            break;
        }
    case GeomAbs_Torus:
        {
            GeomToroid* toroid = new GeomToroid();
            Handle(Geom_ToroidalSurface) this_surf = Handle(Geom_ToroidalSurface)::DownCast
                (toroid->handle());
            this_surf->SetTorus(adapt.Torus());
            surface = new ToroidPy(toroid);
            break;
        }
    case GeomAbs_BezierSurface:
        {
            GeomBezierSurface* surf = new GeomBezierSurface(adapt.Bezier());
            surface = new BezierSurfacePy(surf);
            break;
        }
    case GeomAbs_BSplineSurface:
        {
            GeomBSplineSurface* surf = new GeomBSplineSurface(adapt.BSpline());
            surface = new BSplineSurfacePy(surf);
            break;
        }
    case GeomAbs_SurfaceOfRevolution:
        {
            Handle(Geom_Surface) s = BRep_Tool::Surface(f);
            Handle(Geom_SurfaceOfRevolution) rev = Handle(Geom_SurfaceOfRevolution)::DownCast(s);
            if (rev.IsNull()) {
                Handle(Geom_RectangularTrimmedSurface) rect = Handle(Geom_RectangularTrimmedSurface)::DownCast(s);
                rev = Handle(Geom_SurfaceOfRevolution)::DownCast(rect->BasisSurface());
            }
            if (!rev.IsNull()) {
                GeomSurfaceOfRevolution* surf = new GeomSurfaceOfRevolution(rev);
                surface = new SurfaceOfRevolutionPy(surf);
                break;
            }
            else {
                throw Py::RuntimeError("Failed to convert to surface of revolution");
            }
        }
    case GeomAbs_SurfaceOfExtrusion:
        {
            Handle(Geom_Surface) s = BRep_Tool::Surface(f);
            Handle(Geom_SurfaceOfLinearExtrusion) ext = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(s);
            if (ext.IsNull()) {
                Handle(Geom_RectangularTrimmedSurface) rect = Handle(Geom_RectangularTrimmedSurface)::DownCast(s);
                ext = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(rect->BasisSurface());
            }
            if (!ext.IsNull()) {
                GeomSurfaceOfExtrusion* surf = new GeomSurfaceOfExtrusion(ext);
                surface = new SurfaceOfExtrusionPy(surf);
                break;
            }
            else {
                throw Py::RuntimeError("Failed to convert to surface of extrusion");
            }
        }
    case GeomAbs_OffsetSurface:
        {
            Handle(Geom_Surface) s = BRep_Tool::Surface(f);
            Handle(Geom_OffsetSurface) off = Handle(Geom_OffsetSurface)::DownCast(s);
            if (off.IsNull()) {
                Handle(Geom_RectangularTrimmedSurface) rect = Handle(Geom_RectangularTrimmedSurface)::DownCast(s);
                off = Handle(Geom_OffsetSurface)::DownCast(rect->BasisSurface());
            }
            if (!off.IsNull()) {
                GeomOffsetSurface* surf = new GeomOffsetSurface(off);
                surface = new OffsetSurfacePy(surf);
                break;
            }
            else {
                throw Py::RuntimeError("Failed to convert to offset surface");
            }
        }
    case GeomAbs_OtherSurface:
        break;
    }

    if (surface) {
        surface->setNotTracking();
        return Py::asObject(surface);
    }

    throw Py::TypeError("undefined surface type");
}

Py::Float TopoShapeFacePy::getTolerance(void) const
{
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    return Py::Float(BRep_Tool::Tolerance(f));
}

void TopoShapeFacePy::setTolerance(Py::Float tol)
{
    BRep_Builder aBuilder;
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
    aBuilder.UpdateFace(f, (double)tol);
}

Py::Tuple TopoShapeFacePy::getParameterRange(void) const
{
    const TopoDS_Face& f = TopoDS::Face(getTopoShapePtr()->getShape());
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
    const TopoDS_Shape& clSh = getTopoShapePtr()->getShape();
    if (clSh.IsNull())
        throw Py::RuntimeError("Null shape");
    if (clSh.ShapeType() == TopAbs_FACE) {
        TopoDS_Face clFace = (TopoDS_Face&)clSh;
        TopoDS_Wire clWire = ShapeAnalysis::OuterWire(clFace);
        return Py::Object(new TopoShapeWirePy(new TopoShape(clWire)),true);
    }
    else {
        throw Py::TypeError("Internal error, TopoDS_Shape is not a face!");
    }
}

Py::Object TopoShapeFacePy::getMass(void) const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    double c = props.Mass();
    return Py::Float(c);
}

Py::Object TopoShapeFacePy::getCenterOfMass(void) const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    gp_Pnt c = props.CentreOfMass();
    return Py::Vector(Base::Vector3d(c.X(),c.Y(),c.Z()));
}

Py::Object TopoShapeFacePy::getMatrixOfInertia(void) const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    gp_Mat m = props.MatrixOfInertia();
    Base::Matrix4D mat;
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            mat[i][j] = m(i+1,j+1);
        }
    }
    return Py::Matrix(mat);
}

Py::Object TopoShapeFacePy::getStaticMoments(void) const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    Standard_Real lx,ly,lz;
    props.StaticMoments(lx,ly,lz);
    Py::Tuple tuple(3);
    tuple.setItem(0, Py::Float(lx));
    tuple.setItem(1, Py::Float(ly));
    tuple.setItem(2, Py::Float(lz));
    return tuple;
}

Py::Dict TopoShapeFacePy::getPrincipalProperties(void) const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    GProp_PrincipalProps pprops = props.PrincipalProperties();

    Py::Dict dict;
    dict.setItem("SymmetryAxis", Py::Boolean(pprops.HasSymmetryAxis() ? true : false));
    dict.setItem("SymmetryPoint", Py::Boolean(pprops.HasSymmetryPoint() ? true : false));
    Standard_Real lx,ly,lz;
    pprops.Moments(lx,ly,lz);
    Py::Tuple tuple(3);
    tuple.setItem(0, Py::Float(lx));
    tuple.setItem(1, Py::Float(ly));
    tuple.setItem(2, Py::Float(lz));
    dict.setItem("Moments",tuple);
    dict.setItem("FirstAxisOfInertia",Py::Vector(Base::convertTo
        <Base::Vector3d>(pprops.FirstAxisOfInertia())));
    dict.setItem("SecondAxisOfInertia",Py::Vector(Base::convertTo
        <Base::Vector3d>(pprops.SecondAxisOfInertia())));
    dict.setItem("ThirdAxisOfInertia",Py::Vector(Base::convertTo
        <Base::Vector3d>(pprops.ThirdAxisOfInertia())));

    Standard_Real Rxx,Ryy,Rzz;
    pprops.RadiusOfGyration(Rxx,Ryy,Rzz);
    Py::Tuple rog(3);
    rog.setItem(0, Py::Float(Rxx));
    rog.setItem(1, Py::Float(Ryy));
    rog.setItem(2, Py::Float(Rzz));
    dict.setItem("RadiusOfGyration",rog);
    return dict;
}

PyObject *TopoShapeFacePy::getCustomAttributes(const char* ) const
{
    return 0;
}

int TopoShapeFacePy::setCustomAttributes(const char* , PyObject *)
{
    return 0; 
}
