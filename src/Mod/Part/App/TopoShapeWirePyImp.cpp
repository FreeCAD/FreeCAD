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
# include <Approx_Curve3d.hxx>
# include <ShapeAlgo_AlgoContainer.hxx>
# include <BRepAdaptor_CompCurve.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <Precision.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Wire.hxx>
# include <gp_Ax1.hxx>
#endif

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GCPnts_UniformAbscissa.hxx>

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "BSplineCurvePy.h"
#include "TopoShape.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeWirePy.h"
#include "TopoShapeWirePy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string TopoShapeWirePy::representation(void) const
{
    std::stringstream str;
    str << "<Wire object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapeWirePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of TopoShapeWirePy and the Twin object 
    return new TopoShapeWirePy(new TopoShape);
}

// constructor method
int TopoShapeWirePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pcObj)) {
        BRepBuilderAPI_MakeWire mkWire;
        const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
        if (sh.IsNull()) {
            PyErr_SetString(PyExc_TypeError, "given shape is invalid");
            return -1;
        }
        if (sh.ShapeType() == TopAbs_EDGE)
            mkWire.Add(TopoDS::Edge(sh));
        else if (sh.ShapeType() == TopAbs_WIRE)
            mkWire.Add(TopoDS::Wire(sh));
        else {
            PyErr_SetString(PyExc_TypeError, "shape is neither edge nor wire");
            return -1;
        }

        try {
            getTopoShapePtr()->_Shape = mkWire.Wire();
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return -1;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O", &pcObj)) {
        BRepBuilderAPI_MakeWire mkWire;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->_Shape;
                if (sh.IsNull()) {
                    PyErr_SetString(PyExc_TypeError, "given shape is invalid");
                    return -1;
                }
                if (sh.ShapeType() == TopAbs_EDGE)
                    mkWire.Add(TopoDS::Edge(sh));
                else if (sh.ShapeType() == TopAbs_WIRE)
                    mkWire.Add(TopoDS::Wire(sh));
                else {
                    PyErr_SetString(PyExc_TypeError, "shape is neither edge nor wire");
                    return -1;
                }
            }
            else {
                PyErr_SetString(PyExc_TypeError, "item is not a shape");
                return -1;
            }
        }

        try {
            getTopoShapePtr()->_Shape = mkWire.Wire();
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return -1;
        }
    }

    PyErr_SetString(PyExc_Exception, "edge or wire or list of edges and wires expected");
    return -1;
}

PyObject* TopoShapeWirePy::add(PyObject *args)
{
    PyObject* edge;
    if (!PyArg_ParseTuple(args, "O!",&(TopoShapePy::Type), &edge))
        return 0;
    const TopoDS_Wire& w = TopoDS::Wire(getTopoShapePtr()->_Shape);
    BRepBuilderAPI_MakeWire mkWire(w);

    const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(edge)->getTopoShapePtr()->_Shape;
    if (sh.IsNull()) {
        PyErr_SetString(PyExc_TypeError, "given shape is invalid");
        return 0;
    }
    if (sh.ShapeType() == TopAbs_EDGE)
        mkWire.Add(TopoDS::Edge(sh));
    else if (sh.ShapeType() == TopAbs_WIRE)
        mkWire.Add(TopoDS::Wire(sh));
    else {
        PyErr_SetString(PyExc_TypeError, "shape is neither edge nor wire");
        return 0;
    }

    try {
        getTopoShapePtr()->_Shape = mkWire.Wire();
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeWirePy::fixWire(PyObject *args)
{
    PyObject* face=0;
    double tol = Precision::Confusion();
    if (!PyArg_ParseTuple(args, "|O!d",&(TopoShapeFacePy::Type), &face, &tol))
        return 0;

    try {
        ShapeFix_Wire aFix;
        const TopoDS_Wire& w = TopoDS::Wire(getTopoShapePtr()->_Shape);

        if (face) {
            const TopoDS_Face& f = TopoDS::Face(static_cast<TopoShapePy*>(face)->getTopoShapePtr()->_Shape);
            aFix.Init(w, f, tol);
        }
        else {
            aFix.SetPrecision(tol);
            aFix.Load(w);
        }

        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        getTopoShapePtr()->_Shape = aFix.Wire();

        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeWirePy::makeOffset(PyObject *args)
{
    double dist;
    if (!PyArg_ParseTuple(args, "d",&dist))
        return 0;
    const TopoDS_Wire& w = TopoDS::Wire(getTopoShapePtr()->_Shape);

    BRepOffsetAPI_MakeOffset mkOffset(w);
    mkOffset.Perform(dist);
    
    return new TopoShapePy(new TopoShape(mkOffset.Shape()));
}

PyObject* TopoShapeWirePy::makePipe(PyObject *args)
{
    PyObject *pShape;
    if (PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pShape)) {
        try {
            TopoDS_Shape profile = static_cast<TopoShapePy*>(pShape)->getTopoShapePtr()->_Shape;
            TopoDS_Shape shape = this->getTopoShapePtr()->makePipe(profile);
            return new TopoShapePy(new TopoShape(shape));
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return 0;
        }
    }

    return 0;
}

PyObject* TopoShapeWirePy::makePipeShell(PyObject *args)
{
    PyObject *obj;
    PyObject *make_solid = Py_False;
    PyObject *is_Frenet = Py_False;
    PyObject *transition = Py_False;

    if (PyArg_ParseTuple(args, "O|O!O!O!", &obj,
                             &PyBool_Type, &make_solid,
                             &PyBool_Type, &is_Frenet,
                             &PyBool_Type, &transition)) {
        try {
            TopTools_ListOfShape sections;
            Py::Sequence list(obj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& shape = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->_Shape;
                    sections.Append(shape);
                }
            }
            TopoDS_Shape shape = this->getTopoShapePtr()->makePipeShell(sections, 
                PyObject_IsTrue(make_solid) ? Standard_True : Standard_False,
                PyObject_IsTrue(is_Frenet)  ? Standard_True : Standard_False,
                PyObject_IsTrue(transition) ? Standard_True : Standard_False);
            return new TopoShapePy(new TopoShape(shape));
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return NULL;
        }
    }

    return 0;
}

PyObject* TopoShapeWirePy::makeHomogenousWires(PyObject *args)
{
    PyObject* wire;
    if (!PyArg_ParseTuple(args, "O!",&(Part::TopoShapeWirePy::Type),&wire))
        return 0;
    try {
        TopoDS_Wire o1, o2;
        const TopoDS_Wire& w1 = TopoDS::Wire(getTopoShapePtr()->_Shape);
        const TopoDS_Wire& w2 = TopoDS::Wire(static_cast<TopoShapePy*>(wire)->getTopoShapePtr()->_Shape);
        ShapeAlgo_AlgoContainer shapeAlgo;
        if (shapeAlgo.HomoWires(w1,w2,o1,o2,Standard_True)) {
            getTopoShapePtr()->_Shape = o1;
            return new TopoShapeWirePy(new TopoShape(o2));
        }
        else {
            Py_INCREF(wire);
            return wire;
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeWirePy::approximate(PyObject *args)
{
    double tol2d = gp::Resolution();
    double tol3d = 0.0001;
    int maxseg=10, maxdeg=3;
    if (!PyArg_ParseTuple(args, "ddii",&tol2d,&tol3d,&maxseg,&maxdeg))
        return 0;
    try {
        BRepAdaptor_CompCurve adapt(TopoDS::Wire(getTopoShapePtr()->_Shape));
        Handle_Adaptor3d_HCurve hcurve = adapt.Trim(adapt.FirstParameter(),
                                                    adapt.LastParameter(),
                                                    tol2d);
        Approx_Curve3d approx(hcurve, tol3d, GeomAbs_C0, maxseg, maxdeg);
        if (approx.IsDone()) {
            return new BSplineCurvePy(new GeomBSplineCurve(approx.Curve()));
        }
        else {
            PyErr_SetString(PyExc_Exception, "failed to approximate wire");
            return 0;
        }
    }
    catch (Standard_Failure) {
        PyErr_SetString(PyExc_Exception, "failed to approximate wire");
        return 0;
    }
}

PyObject* TopoShapeWirePy::discretize(PyObject *args)
{
    PyObject* defl_or_num;
    if (!PyArg_ParseTuple(args, "O", &defl_or_num))
        return 0;

    try {
        BRepAdaptor_CompCurve adapt(TopoDS::Wire(getTopoShapePtr()->_Shape));
        GCPnts_UniformAbscissa discretizer;
        if (PyInt_Check(defl_or_num)) {
            int num = PyInt_AsLong(defl_or_num);
            discretizer.Initialize (adapt, num);
        }
        else if (PyFloat_Check(defl_or_num)) {
            double defl = PyFloat_AsDouble(defl_or_num);
            discretizer.Initialize (adapt, defl);
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Either int or float expected");
            return 0;
        }
        if (discretizer.IsDone () && discretizer.NbPoints () > 0) {
            Py::List points;
            int nbPoints = discretizer.NbPoints ();
            for (int i=1; i<=nbPoints; i++) {
                gp_Pnt p = adapt.Value (discretizer.Parameter (i));
                points.append(Py::Vector(Base::Vector3d(p.X(),p.Y(),p.Z())));
            }

            return Py::new_reference_to(points);
        }
        else {
            PyErr_SetString(PyExc_Exception, "Descretization of wire failed");
            return 0;
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    PyErr_SetString(PyExc_Exception, "Geometry is not a curve");
    return 0;
}

Py::Object TopoShapeWirePy::getCenterOfMass(void) const
{
    GProp_GProps props;
    BRepGProp::LinearProperties(getTopoShapePtr()->_Shape, props);
    gp_Pnt c = props.CentreOfMass();
    return Py::Vector(Base::Vector3d(c.X(),c.Y(),c.Z()));
}

PyObject *TopoShapeWirePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TopoShapeWirePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


