/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Geom2d_BezierCurve.hxx>
# include <gp_Pnt2d.hxx>
# include <TColgp_Array1OfPnt2d.hxx>
# include <TColStd_Array1OfReal.hxx>
#endif

#include <Base/GeometryPyCXX.h>

#include "Geom2d/BezierCurve2dPy.h"
#include "Geom2d/BezierCurve2dPy.cpp"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BezierCurve2dPy::representation() const
{
    return "<BezierCurve2d object>";
}

PyObject *BezierCurve2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BezierCurve2dPy and the Twin object
    return new BezierCurve2dPy(new Geom2dBezierCurve);
}

// constructor method
int BezierCurve2dPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* BezierCurve2dPy::isRational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
        (getGeometry2dPtr()->handle());
    Standard_Boolean val = curve->IsRational();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierCurve2dPy::isPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
        (getGeometry2dPtr()->handle());
    Standard_Boolean val = curve->IsPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierCurve2dPy::isClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
        (getGeometry2dPtr()->handle());
    Standard_Boolean val = curve->IsClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierCurve2dPy::increase(PyObject * args)
{
    int degree;
    if (!PyArg_ParseTuple(args, "i", &degree))
        return nullptr;
    Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
        (getGeometry2dPtr()->handle());
    curve->Increase(degree);
    Py_Return;
}

PyObject* BezierCurve2dPy::insertPoleAfter(PyObject * args)
{
    int index;
    double weight=1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iO!|d", &index, Base::Vector2dPy::type_object(), &p, &weight))
        return nullptr;
    Base::Vector2d vec = Py::toVector2d(p);
    gp_Pnt2d pnt(vec.x, vec.y);
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->InsertPoleAfter(index,pnt,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::insertPoleBefore(PyObject * args)
{
    int index;
    double weight=1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iO!|d", &index, Base::Vector2dPy::type_object(), &p, &weight))
        return nullptr;
    Base::Vector2d vec = Py::toVector2d(p);
    gp_Pnt2d pnt(vec.x, vec.y);
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->InsertPoleBefore(index,pnt,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::removePole(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->RemovePole(index);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::segment(PyObject * args)
{
    double u1,u2;
    if (!PyArg_ParseTuple(args, "dd", &u1,&u2))
        return nullptr;
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->Segment(u1,u2);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::setPole(PyObject * args)
{
    int index;
    double weight=-1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iO!|d", &index, Base::Vector2dPy::type_object(), &p, &weight))
        return nullptr;
    Base::Vector2d vec = Py::toVector2d(p);
    gp_Pnt2d pnt(vec.x, vec.y);
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        if (weight < 0.0)
            curve->SetPole(index,pnt);
        else
            curve->SetPole(index,pnt,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::getPole(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        Standard_OutOfRange_Raise_if
            (index < 1 || index > curve->NbPoles(), "Pole index out of range");
        gp_Pnt2d pnt = curve->Pole(index);
        return Py::new_reference_to(Base::Vector2dPy::create(pnt.X(), pnt.Y()));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::getPoles(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        TColgp_Array1OfPnt2d p(1,curve->NbPoles());
        curve->Poles(p);
        Py::List poles;

        for (Standard_Integer i=p.Lower(); i<=p.Upper(); i++) {
            gp_Pnt2d pnt = p(i);
            poles.append(Base::Vector2dPy::create(pnt.X(), pnt.Y()));
        }
        return Py::new_reference_to(poles);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::setPoles(PyObject * args)
{
    PyObject* plist;
    if (!PyArg_ParseTuple(args, "O", &plist))
        return nullptr;
    try {
        Py::Sequence list(plist);
        TColgp_Array1OfPnt2d poles(1,list.size());
        int index = poles.Lower();
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Base::Vector2d pole = Py::toVector2d(*it);
            poles.SetValue(index++, gp_Pnt2d(pole.x,pole.y));
        }

        Handle(Geom2d_BezierCurve) bezier = new Geom2d_BezierCurve(poles);
        this->getGeom2dBezierCurvePtr()->setHandle(bezier);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::setWeight(PyObject * args)
{
    int index;
    double weight;
    if (!PyArg_ParseTuple(args, "id", &index,&weight))
        return nullptr;
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->SetWeight(index,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::getWeight(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        Standard_OutOfRange_Raise_if
            (index < 1 || index > curve->NbPoles() , "Weight index out of range");
        double weight = curve->Weight(index);
        return Py_BuildValue("d", weight);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::getWeights(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        TColStd_Array1OfReal w(1,curve->NbPoles());
        curve->Weights(w);
        Py::List weights;
        for (Standard_Integer i=w.Lower(); i<=w.Upper(); i++) {
            weights.append(Py::Float(w(i)));
        }
        return Py::new_reference_to(weights);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurve2dPy::getResolution(PyObject* args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return nullptr;
    try {
        Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
            (getGeometry2dPtr()->handle());
        double utol;
        curve->Resolution(tol,utol);
        return Py_BuildValue("d",utol);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}
Py::Long BezierCurve2dPy::getDegree() const
{
    Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
        (getGeometry2dPtr()->handle());
    return Py::Long(curve->Degree());
}

Py::Long BezierCurve2dPy::getMaxDegree() const
{
    Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
        (getGeometry2dPtr()->handle());
    return Py::Long(curve->MaxDegree());
}

Py::Long BezierCurve2dPy::getNbPoles() const
{
    Handle(Geom2d_BezierCurve) curve = Handle(Geom2d_BezierCurve)::DownCast
        (getGeometry2dPtr()->handle());
    return Py::Long(curve->NbPoles());
}

Py::Object BezierCurve2dPy::getStartPoint() const
{
    Handle(Geom2d_BezierCurve) c = Handle(Geom2d_BezierCurve)::DownCast
        (getGeometry2dPtr()->handle());
    gp_Pnt2d pnt = c->StartPoint();
    return Base::Vector2dPy::create(pnt.X(), pnt.Y());
}

Py::Object BezierCurve2dPy::getEndPoint() const
{
    Handle(Geom2d_BezierCurve) c = Handle(Geom2d_BezierCurve)::DownCast
        (getGeometry2dPtr()->handle());
    gp_Pnt2d pnt = c->EndPoint();
    return Base::Vector2dPy::create(pnt.X(), pnt.Y());
}

PyObject *BezierCurve2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BezierCurve2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
