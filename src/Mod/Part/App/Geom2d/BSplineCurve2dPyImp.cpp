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
# include <Geom2d_BSplineCurve.hxx>
# include <Geom2dAPI_PointsToBSpline.hxx>
# include <Geom2dAPI_Interpolate.hxx>
# include <Geom2dConvert_BSplineCurveToBezierCurve.hxx>
# include <Standard_PrimitiveTypes.hxx>
# include <gp_Pnt2d.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColgp_Array1OfPnt2d.hxx>
# include <TColgp_Array1OfVec2d.hxx>
# include <TColgp_HArray1OfPnt2d.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <TColStd_HArray1OfReal.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_HArray1OfBoolean.hxx>

# include <Precision.hxx>
#endif

#include <Base/GeometryPyCXX.h>

#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Part/App/Geom2d/BSplineCurve2dPy.h>
#include <Mod/Part/App/Geom2d/BSplineCurve2dPy.cpp>
#include <Mod/Part/App/Geom2d/BezierCurve2dPy.h>

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BSplineCurve2dPy::representation(void) const
{
    return "<BSplineCurve2d object>";
}

PyObject *BSplineCurve2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BSplineCurve2dPy and the Twin object
    return new BSplineCurve2dPy(new Geom2dBSplineCurve);
}

// constructor method
int BSplineCurve2dPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "B-spline constructor accepts:\n"
        "-- empty parameter list\n");
    return -1;
}

PyObject* BSplineCurve2dPy::isRational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    Standard_Boolean val = curve->IsRational();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineCurve2dPy::isPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    Standard_Boolean val = curve->IsPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineCurve2dPy::isClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    Standard_Boolean val = curve->IsClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineCurve2dPy::increaseDegree(PyObject * args)
{
    int degree;
    if (!PyArg_ParseTuple(args, "i", &degree))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->IncreaseDegree(degree);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::increaseMultiplicity(PyObject * args)
{
    int mult=-1;
    int start, end;
    if (!PyArg_ParseTuple(args, "ii|i", &start, &end, &mult))
        return 0;

    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        if (mult == -1) {
            mult = end;
            curve->IncreaseMultiplicity(start, mult);
        }
        else {
            curve->IncreaseMultiplicity(start, end, mult);
        }

        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::incrementMultiplicity(PyObject * args)
{
    int start, end, mult;
    if (!PyArg_ParseTuple(args, "iii", &start, &end, &mult))
        return 0;

    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->IncrementMultiplicity(start, end, mult);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineCurve2dPy::insertKnot(PyObject * args)
{
    double U, tol = 0.0;
    int M=1;
    if (!PyArg_ParseTuple(args, "d|idO!", &U, &M, &tol))
        return 0;

    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->InsertKnot(U,M,tol);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineCurve2dPy::insertKnots(PyObject * args)
{
    double tol = 0.0;
    PyObject* add = Py_True;
    PyObject* obj1;
    PyObject* obj2;
    if (!PyArg_ParseTuple(args, "OO|dO!", &obj1,
                                          &obj2,
                                          &tol, &PyBool_Type, &add))
        return 0;

    try {
        Py::Sequence knots(obj1);
        TColStd_Array1OfReal k(1,knots.size());
        int index=1;
        for (Py::Sequence::iterator it = knots.begin(); it != knots.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }
        Py::Sequence mults(obj2);
        TColStd_Array1OfInteger m(1,mults.size());
        index=1;
        for (Py::Sequence::iterator it = mults.begin(); it != mults.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
            Py::Long val(*it);
#else
            Py::Int val(*it);
#endif
            m(index++) = (int)val;
        }

        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->InsertKnots(k,m,tol,PyObject_IsTrue(add) ? Standard_True : Standard_False);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineCurve2dPy::removeKnot(PyObject * args)
{
    double tol;
    int Index,M;
    if (!PyArg_ParseTuple(args, "iid", &Index, &M, &tol))
        return 0;

    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        Standard_Boolean ok = curve->RemoveKnot(Index,M,tol);
        return PyBool_FromLong(ok ? 1 : 0);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::segment(PyObject * args)
{
    double u1,u2;
    if (!PyArg_ParseTuple(args, "dd", &u1,&u2))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->Segment(u1,u2);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::setKnot(PyObject * args)
{
    int Index, M=-1;
    double K;
    if (!PyArg_ParseTuple(args, "id|i", &Index, &K, &M))
        return 0;

    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    if (M == -1) {
        curve->SetKnot(Index, K);
    }
    else {
        curve->SetKnot(Index, K, M);
    }

    Py_Return;
}

PyObject* BSplineCurve2dPy::getKnot(PyObject * args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return 0;

    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    double M = curve->Knot(Index);

    return Py_BuildValue("d",M);
}

PyObject* BSplineCurve2dPy::setKnots(PyObject * args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return 0;
    try {
        Py::Sequence list(obj);
        TColStd_Array1OfReal k(1,list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }

        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->SetKnots(k);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getKnots(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        TColStd_Array1OfReal w(1,curve->NbKnots());
        curve->Knots(w);
        Py::List knots;
        for (Standard_Integer i=w.Lower(); i<=w.Upper(); i++) {
            knots.append(Py::Float(w(i)));
        }
        return Py::new_reference_to(knots);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::setPole(PyObject * args)
{
    int index;
    double weight=-1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iO!|d", &index, Base::Vector2dPy::type_object(), &p, &weight))
        return 0;
    Base::Vector2d vec = Py::toVector2d(p);
    gp_Pnt2d pnt(vec.x, vec.y);
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        if (weight < 0.0)
            curve->SetPole(index,pnt);
        else
            curve->SetPole(index,pnt,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getPole(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        Standard_OutOfRange_Raise_if
            (index < 1 || index > curve->NbPoles(), "Pole index out of range");
        gp_Pnt2d pnt = curve->Pole(index);

        Py::Module module("__FreeCADBase__");
        Py::Callable method(module.getAttr("Vector2d"));
        Py::Tuple arg(2);
        arg.setItem(0, Py::Float(pnt.X()));
        arg.setItem(1, Py::Float(pnt.Y()));
        return Py::new_reference_to(method.apply(arg));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getPoles(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        TColgp_Array1OfPnt2d p(1, (int)curve->NbPoles());
        curve->Poles(p);

        Py::List poles;
        Py::Module module("__FreeCADBase__");
        Py::Callable method(module.getAttr("Vector2d"));
        Py::Tuple arg(2);
        for (Standard_Integer i=p.Lower(); i<=p.Upper(); i++) {
            gp_Pnt2d pnt = p(i);

            arg.setItem(0, Py::Float(pnt.X()));
            arg.setItem(1, Py::Float(pnt.Y()));
            poles.append(method.apply(arg));
        }
        return Py::new_reference_to(poles);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getPolesAndWeights(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        TColgp_Array1OfPnt2d p(1,curve->NbPoles());
        curve->Poles(p);
        TColStd_Array1OfReal w(1,curve->NbPoles());
        curve->Weights(w);

        Py::List poles;
        for (Standard_Integer i=p.Lower(); i<=p.Upper(); i++) {
            gp_Pnt2d pnt = p(i);
            double weight = w(i);
            Py::Tuple t(3);
            t.setItem(0, Py::Float(pnt.X()));
            t.setItem(1, Py::Float(pnt.Y()));
            t.setItem(2, Py::Float(weight));
            poles.append(t);
        }
        return Py::new_reference_to(poles);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::setWeight(PyObject * args)
{
    int index;
    double weight;
    if (!PyArg_ParseTuple(args, "id", &index,&weight))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->SetWeight(index,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getWeight(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        Standard_OutOfRange_Raise_if
            (index < 1 || index > curve->NbPoles() , "Weight index out of range");
        double weight = curve->Weight(index);
        return Py_BuildValue("d", weight);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getWeights(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
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
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getResolution(PyObject * args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        double utol;
        curve->Resolution(tol,utol);
        return Py_BuildValue("d",utol);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::movePoint(PyObject * args)
{
    double U;
    int index1, index2;
    PyObject* pnt;
    if (!PyArg_ParseTuple(args, "dO!ii", &U, Base::Vector2dPy::type_object(),&pnt, &index1, &index2))
        return 0;
    try {
        Base::Vector2d p = Py::toVector2d(pnt);
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        int first, last;
        curve->MovePoint(U, gp_Pnt2d(p.x,p.y), index1, index2, first, last);
        return Py_BuildValue("(ii)",first, last);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::setNotPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->SetNotPeriodic();
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::setPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->SetPeriodic();
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::setOrigin(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        curve->SetOrigin(index);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getMultiplicity(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        int mult = curve->Multiplicity(index);
        return Py_BuildValue("i", mult);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getMultiplicities(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
            (getGeometry2dPtr()->handle());
        TColStd_Array1OfInteger m(1,curve->NbKnots());
        curve->Multiplicities(m);
        Py::List mults;
        for (Standard_Integer i=m.Lower(); i<=m.Upper(); i++) {
#if PY_MAJOR_VERSION >= 3
            mults.append(Py::Long(m(i)));
#else
            mults.append(Py::Int(m(i)));
#endif
        }
        return Py::new_reference_to(mults);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

Py::Long BSplineCurve2dPy::getDegree(void) const
{
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    return Py::Long(curve->Degree()); 
}

Py::Long BSplineCurve2dPy::getMaxDegree(void) const
{
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    return Py::Long(curve->MaxDegree()); 
}

Py::Long BSplineCurve2dPy::getNbPoles(void) const
{
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    return Py::Long(curve->NbPoles()); 
}

Py::Long BSplineCurve2dPy::getNbKnots(void) const
{
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    return Py::Long(curve->NbKnots()); 
}

Py::Object BSplineCurve2dPy::getStartPoint(void) const
{
    Handle(Geom2d_BSplineCurve) c = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    gp_Pnt2d pnt = c->StartPoint();

    Py::Module module("__FreeCADBase__");
    Py::Callable method(module.getAttr("Vector2d"));
    Py::Tuple arg(2);
    arg.setItem(0, Py::Float(pnt.X()));
    arg.setItem(1, Py::Float(pnt.Y()));
    return method.apply(arg);
}

Py::Object BSplineCurve2dPy::getEndPoint(void) const
{
    Handle(Geom2d_BSplineCurve) c = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    gp_Pnt2d pnt = c->EndPoint();

    Py::Module module("__FreeCADBase__");
    Py::Callable method(module.getAttr("Vector2d"));
    Py::Tuple arg(2);
    arg.setItem(0, Py::Float(pnt.X()));
    arg.setItem(1, Py::Float(pnt.Y()));
    return method.apply(arg);
}

Py::Object BSplineCurve2dPy::getFirstUKnotIndex(void) const
{
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
#if PY_MAJOR_VERSION >= 3
    return Py::Long(curve->FirstUKnotIndex()); 
#else
    return Py::Int(curve->FirstUKnotIndex()); 
#endif
}

Py::Object BSplineCurve2dPy::getLastUKnotIndex(void) const
{
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
#if PY_MAJOR_VERSION >= 3
    return Py::Long(curve->LastUKnotIndex()); 
#else
    return Py::Int(curve->FirstUKnotIndex()); 
#endif
}

Py::List BSplineCurve2dPy::getKnotSequence(void) const
{
    Handle(Geom2d_BSplineCurve) curve = Handle(Geom2d_BSplineCurve)::DownCast
        (getGeometry2dPtr()->handle());
    Standard_Integer m = 0;
    if (curve->IsPeriodic()) {
        // knots=poles+2*degree-mult(1)+2
        m = (int)(curve->NbPoles() + 2*curve->Degree() - curve->Multiplicity(1) + 2);
    }
    else {
        // knots=poles+degree+1
        for (int i=1; i<= curve->NbKnots(); i++)
            m += (int)curve->Multiplicity(i);
    }

    TColStd_Array1OfReal k(1,m);
    curve->KnotSequence(k);
    Py::List list;
    for (Standard_Integer i=k.Lower(); i<=k.Upper(); i++) {
        list.append(Py::Float(k(i)));
    }
    return list;
}

PyObject* BSplineCurve2dPy::toBiArcs(PyObject * args)
{
    double tolerance = 0.001;
    if (!PyArg_ParseTuple(args, "d", &tolerance))
        return 0;
    try {
        Geom2dBSplineCurve* curve = getGeom2dBSplineCurvePtr();
        std::list<Geometry2d*> arcs;
        arcs = curve->toBiArcs(tolerance);

        Py::List list;
        for (std::list<Geometry2d*>::iterator it = arcs.begin(); it != arcs.end(); ++it) {
            list.append(Py::asObject((*it)->getPyObject()));
            delete (*it);
        }

        return Py::new_reference_to(list);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::approximate(PyObject *args, PyObject *kwds)
{
    PyObject* obj;
    Standard_Integer degMin=3;
    Standard_Integer degMax=8;
    char* continuity = "C2";
    double tol3d = 1e-3;
    char* parType = "ChordLength";
    PyObject* par = 0;
    double weight1 = 0;
    double weight2 = 0;
    double weight3 = 0;
    
    static char* kwds_interp[] = {"Points", "DegMax", "Continuity", "Tolerance", "DegMin", "ParamType", "Parameters",
                                  "LengthWeight", "CurvatureWeight", "TorsionWeight", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|isdisOddd",kwds_interp,
                                     &obj, &degMax,
                                     &continuity, &tol3d, &degMin, 
                                     &parType, &par,
                                     &weight1, &weight2, &weight3))
        return 0;
    
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt2d pnts(1,list.size());
        Standard_Integer index = 1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Base::Vector2d vec = Py::toVector2d(*it);
            pnts(index++) = gp_Pnt2d(vec.x,vec.y);
        }

        if (degMin > degMax) {
            Standard_Failure::Raise("DegMin must be lower or equal to DegMax");
        }
        
        GeomAbs_Shape c;
        std::string str = continuity;
        if (str == "C0")
            c = GeomAbs_C0;
        else if (str == "G1")
            c = GeomAbs_G1;
        else if (str == "C1")
            c = GeomAbs_C1;
        else if (str == "G2")
            c = GeomAbs_G2;
        else if (str == "C2")
            c = GeomAbs_C2;
        else if (str == "C3")
            c = GeomAbs_C3;
        else if (str == "CN")
            c = GeomAbs_CN;
        else
            c = GeomAbs_C2;
        
        if (weight1 || weight2 || weight3) {
            // It seems that this function only works with Continuity = C0, C1 or C2
            if (!(c == GeomAbs_C0 || c == GeomAbs_C1 || c == GeomAbs_C2)) {
                c = GeomAbs_C2;
            }

            Geom2dAPI_PointsToBSpline fit(pnts, weight1, weight2, weight3, degMax, c, tol3d);
            Handle(Geom2d_BSplineCurve) spline = fit.Curve();
            if (!spline.IsNull()) {
                this->getGeom2dBSplineCurvePtr()->setHandle(spline);
                Py_Return;
            }
            else {
                Standard_Failure::Raise("Smoothing approximation failed");
                return 0; // goes to the catch block
            }
        }
        
        if (par) {
            Py::Sequence plist(par);
            TColStd_Array1OfReal parameters(1,plist.size());
            Standard_Integer index = 1;
            for (Py::Sequence::iterator it = plist.begin(); it != plist.end(); ++it) {
                Py::Float f(*it);
                parameters(index++) = static_cast<double>(f);
            }
            
            Geom2dAPI_PointsToBSpline fit(pnts, parameters, degMin, degMax, c, tol3d);
            Handle(Geom2d_BSplineCurve) spline = fit.Curve();
            if (!spline.IsNull()) {
                this->getGeom2dBSplineCurvePtr()->setHandle(spline);
                Py_Return;
            }
            else {
                Standard_Failure::Raise("Approximation with parameters failed");
                return 0; // goes to the catch block
            }
        }
        
        Approx_ParametrizationType pt;
        std::string pstr = parType;
        if (pstr == "Uniform")
            pt = Approx_IsoParametric;
        else if (pstr == "Centripetal")
            pt = Approx_Centripetal;
        else
            pt = Approx_ChordLength;

        Geom2dAPI_PointsToBSpline fit(pnts, pt, degMin, degMax, c, tol3d);
        Handle(Geom2d_BSplineCurve) spline = fit.Curve();
        if (!spline.IsNull()) {
            this->getGeom2dBSplineCurvePtr()->setHandle(spline);
            Py_Return;
        }
        else {
            Standard_Failure::Raise("failed to approximate points");
            return 0; // goes to the catch block
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getCardinalSplineTangents(PyObject *args, PyObject *kwds)
{
    PyObject* pts;
    PyObject* tgs;
    double parameter;

    static char* kwds_interp1[] = {"Points", "Parameter", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "Od",kwds_interp1, &pts, &parameter)) {
        Py::Sequence list(pts);
        std::vector<gp_Pnt2d> interpPoints;
        interpPoints.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Base::Vector2d pnt = Py::toVector2d(*it);
            interpPoints.push_back(gp_Pnt2d(pnt.x,pnt.y));
        }

        Geom2dBSplineCurve* bspline = this->getGeom2dBSplineCurvePtr();
        std::vector<gp_Vec2d> tangents;
        bspline->getCardinalSplineTangents(interpPoints, parameter, tangents);

        Py::List vec;
        Py::Module module("__FreeCADBase__");
        Py::Callable method(module.getAttr("Vector2d"));
        Py::Tuple arg(2);
        for (gp_Vec2d it : tangents) {
            arg.setItem(0, Py::Float(it.X()));
            arg.setItem(1, Py::Float(it.Y()));
            vec.append(method.apply(arg));
        }
        return Py::new_reference_to(vec);
    }

    PyErr_Clear();
    static char* kwds_interp2[] = {"Points", "Parameters", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "OO",kwds_interp2, &pts, &tgs)) {
        Py::Sequence list(pts);
        std::vector<gp_Pnt2d> interpPoints;
        interpPoints.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Base::Vector2d pnt = Py::toVector2d(*it);
            interpPoints.push_back(gp_Pnt2d(pnt.x,pnt.y));
        }

        Py::Sequence list2(tgs);
        std::vector<double> parameters;
        parameters.reserve(list2.size());
        for (Py::Sequence::iterator it = list2.begin(); it != list2.end(); ++it) {
            Py::Float p(*it);
            parameters.push_back(static_cast<double>(p));
        }

        Geom2dBSplineCurve* bspline = this->getGeom2dBSplineCurvePtr();
        std::vector<gp_Vec2d> tangents;
        bspline->getCardinalSplineTangents(interpPoints, parameters, tangents);

        Py::List vec;
        Py::Module module("__FreeCADBase__");
        Py::Callable method(module.getAttr("Vector2d"));
        Py::Tuple arg(2);
        for (gp_Vec2d it : tangents) {
            arg.setItem(0, Py::Float(it.X()));
            arg.setItem(1, Py::Float(it.Y()));
            vec.append(method.apply(arg));
        }
        return Py::new_reference_to(vec);
    }

    return 0;
}

PyObject* BSplineCurve2dPy::interpolate(PyObject *args, PyObject *kwds)
{
    PyObject* obj;
    PyObject* par = 0;
    double tol3d = Precision::Approximation();
    PyObject* periodic = Py_False;
    PyObject* t1 = 0; PyObject* t2 = 0;
    PyObject* ts = 0; PyObject* fl = 0;

    static char* kwds_interp[] = {"Points", "PeriodicFlag", "Tolerance", "InitialTangent", "FinalTangent",
                                  "Tangents", "TangentFlags", "Parameters", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O!dO!O!OOO",kwds_interp,
                                     &obj, &PyBool_Type, &periodic, &tol3d,
                                     Base::Vector2dPy::type_object(), &t1,
                                     Base::Vector2dPy::type_object(), &t2,
                                     &ts, &fl, &par))
        return 0;

    try {
        Py::Sequence list(obj);
        Handle(TColgp_HArray1OfPnt2d) interpolationPoints = new TColgp_HArray1OfPnt2d(1, list.size());
        Standard_Integer index = 1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Base::Vector2d pnt = Py::toVector2d(*it);
            interpolationPoints->SetValue(index++, gp_Pnt2d(pnt.x,pnt.y));
        }

        if (interpolationPoints->Length() < 2) {
            Standard_Failure::Raise("not enough points given");
        }

        Handle(TColStd_HArray1OfReal) parameters;
        if (par) {
            Py::Sequence plist(par);
            parameters = new TColStd_HArray1OfReal(1, plist.size());
            Standard_Integer pindex = 1;
            for (Py::Sequence::iterator it = plist.begin(); it != plist.end(); ++it) {
                Py::Float f(*it);
                parameters->SetValue(pindex++, static_cast<double>(f));
            }
        }

        std::unique_ptr<Geom2dAPI_Interpolate> aBSplineInterpolation;
        if (parameters.IsNull()) {
            aBSplineInterpolation.reset(new Geom2dAPI_Interpolate(interpolationPoints,
                PyObject_IsTrue(periodic) ? Standard_True : Standard_False, tol3d));
        }
        else {
            aBSplineInterpolation.reset(new Geom2dAPI_Interpolate(interpolationPoints, parameters,
                PyObject_IsTrue(periodic) ? Standard_True : Standard_False, tol3d));
        }

        if (t1 && t2) {
            Base::Vector2d v1 = Py::toVector2d(t1);
            Base::Vector2d v2 = Py::toVector2d(t2);
            gp_Vec2d initTangent(v1.x,v1.y), finalTangent(v2.x,v2.y);
            aBSplineInterpolation->Load(initTangent, finalTangent);
        }
        else if (ts && fl) {
            Py::Sequence tlist(ts);
            TColgp_Array1OfVec2d tangents(1, tlist.size());
            Standard_Integer index = 1;
            for (Py::Sequence::iterator it = tlist.begin(); it != tlist.end(); ++it) {
                Base::Vector2d vec = Py::toVector2d(*it);
                tangents.SetValue(index++, gp_Vec2d(vec.x,vec.y));
            }

            Py::Sequence flist(fl);
            Handle(TColStd_HArray1OfBoolean) tangentFlags = new TColStd_HArray1OfBoolean(1, flist.size());
            Standard_Integer findex = 1;
            for (Py::Sequence::iterator it = flist.begin(); it != flist.end(); ++it) {
                Py::Boolean flag(*it);
                tangentFlags->SetValue(findex++, static_cast<bool>(flag) ? Standard_True : Standard_False);
            }

            aBSplineInterpolation->Load(tangents, tangentFlags);
        }

        aBSplineInterpolation->Perform();
        if (aBSplineInterpolation->IsDone()) {
            Handle(Geom2d_BSplineCurve) aBSplineCurve(aBSplineInterpolation->Curve());
            this->getGeom2dBSplineCurvePtr()->setHandle(aBSplineCurve);
            Py_Return;
        }
        else {
            Standard_Failure::Raise("failed to interpolate points");
            return 0; // goes to the catch block
        }
    }
    catch (Standard_Failure& e) {

        std::string err = e.GetMessageString();
        if (err.empty()) err = e.DynamicType()->Name();
        PyErr_SetString(PartExceptionOCCError, err.c_str());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::buildFromPoles(PyObject *args)
{
    PyObject* obj;
    int degree = 3;
    PyObject* periodic = Py_False;
    PyObject* interpolate = Py_False;
    if (!PyArg_ParseTuple(args, "O|O!iO!",&obj, &PyBool_Type, &periodic, &degree, &PyBool_Type, interpolate))
        return 0;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt2d poles(1, list.size());
        Standard_Integer index = 1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Base::Vector2d pnt = Py::toVector2d(*it);
            poles(index++) = gp_Pnt2d(pnt.x,pnt.y);
        }

        if (poles.Length() <= degree)
            degree = poles.Length()-1;

        if (PyObject_IsTrue(periodic)) {
            int mult;
            int len;
            if (PyObject_IsTrue(interpolate)) {
                mult = degree;
                len = poles.Length() - mult + 2;
            }
            else {
                mult = 1;
                len = poles.Length() + 1;
            }
            TColStd_Array1OfReal knots(1, len);
            TColStd_Array1OfInteger mults(1, len);
            for (int i=1; i<=knots.Length(); i++){
                knots.SetValue(i,(double)(i-1)/(knots.Length()-1));
                mults.SetValue(i,1);
            }
            mults.SetValue(1, mult);
            mults.SetValue(knots.Length(), mult);

            Handle(Geom2d_BSplineCurve) spline = new Geom2d_BSplineCurve(poles, knots, mults, degree, Standard_True);
            if (!spline.IsNull()) {
                this->getGeom2dBSplineCurvePtr()->setHandle(spline);
                Py_Return;
            }
            else {
                Standard_Failure::Raise("failed to create spline");
                return 0; // goes to the catch block
            }
        }
        else {
            TColStd_Array1OfReal knots(1, poles.Length()+degree+1-2*(degree));
            TColStd_Array1OfInteger mults(1, poles.Length()+degree+1-2*(degree));
            for (int i=1; i<=knots.Length(); i++){
                knots.SetValue(i,(double)(i-1)/(knots.Length()-1));
                mults.SetValue(i,1);
            }
            mults.SetValue(1, degree+1);
            mults.SetValue(knots.Length(), degree+1);

            Handle(Geom2d_BSplineCurve) spline = new Geom2d_BSplineCurve(poles, knots, mults, degree, Standard_False);
            if (!spline.IsNull()) {
                this->getGeom2dBSplineCurvePtr()->setHandle(spline);
                Py_Return;
            }
            else {
                Standard_Failure::Raise("failed to create spline");
                return 0; // goes to the catch block
            }
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::buildFromPolesMultsKnots(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"poles", "mults", "knots", "periodic", "degree", "weights", NULL};
    PyObject* periodic = Py_False;
    PyObject* poles = Py_None;
    PyObject* mults = Py_None;
    PyObject* knots = Py_None;
    PyObject* weights = Py_None;
    int degree = 3;
    int number_of_poles = 0;
    int number_of_knots = 0;
    int sum_of_mults = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OOO!iO", kwlist,
        &poles, &mults, &knots,
        &PyBool_Type, &periodic,
        &degree, &weights))
        return 0;
    try {
        // poles have to be present
        Py::Sequence list(poles);

        number_of_poles = list.size();
        if ((number_of_poles) < 2) {
            Standard_Failure::Raise("need two or more poles");
            return 0;
        }
        TColgp_Array1OfPnt2d occpoles(1, number_of_poles);
        Standard_Integer index = 1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Base::Vector2d pnt = Py::toVector2d(*it);
            occpoles(index++) = gp_Pnt2d(pnt.x,pnt.y);
        }
        //Calculate the number of knots
        if (mults != Py_None && knots != Py_None) {
            number_of_knots = PyObject_Length(mults);
            if (PyObject_Length(knots) != number_of_knots) {
                Standard_Failure::Raise("number of knots and mults mismatch");
                return 0;
            }
        }
        else {
            if (mults != Py_None) {
                number_of_knots = PyObject_Length(mults);
            }
            else {
                if (knots != Py_None) { number_of_knots = PyObject_Length(knots); }
                else { //guess number of knots
                    if (PyObject_IsTrue(periodic)) {
                        if (number_of_poles < degree) {degree = number_of_poles+1;}
                        number_of_knots = number_of_poles+1;
                    }
                    else {
                        if (number_of_poles <= degree) {degree = number_of_poles-1;}
                        number_of_knots = number_of_poles-degree+1;
                    }
                }
            }
        }
        TColStd_Array1OfInteger occmults(1,number_of_knots);
        TColStd_Array1OfReal occknots(1,number_of_knots);
        TColStd_Array1OfReal occweights(1,number_of_poles);
        if (mults != Py_None) { //mults are given
            Py::Sequence multssq(mults);
            Standard_Integer index = 1;
            for (Py::Sequence::iterator it = multssq.begin(); it != multssq.end() && index <= occmults.Length(); ++it) {
#if PY_MAJOR_VERSION >=3
                Py::Long mult(*it);
#else
                Py::Int mult(*it);
#endif
                if (index < occmults.Length() || PyObject_Not(periodic)) {
                    sum_of_mults += (int)mult; //sum up the mults to compare them against the number of poles later
                }
                occmults(index++) = mult;
            }
        }
        else { //mults are 1 or degree+1 at the ends
            for (int i=1; i<=occmults.Length(); i++){
                occmults.SetValue(i,1);
            }
            if (PyObject_Not(periodic) && occmults.Length() > 0) {
                occmults.SetValue(1, degree+1);
                occmults.SetValue(occmults.Length(), degree+1);
                sum_of_mults = occmults.Length()+2*degree;
            }
            else { sum_of_mults = occmults.Length()-1;}
        }
        if (knots != Py_None) { //knots are given
            Py::Sequence knotssq(knots);
            index = 1;
            for (Py::Sequence::iterator it = knotssq.begin(); it != knotssq.end() && index <= occknots.Length(); ++it) {
                Py::Float knot(*it);
                occknots(index++) = knot;
            }
        }
        else { // knotes are uniformly spaced 0..1 if not given
            for (int i=1; i<=occknots.Length(); i++){
                occknots.SetValue(i,(double)(i-1)/(occknots.Length()-1));
            }
        }
        if (weights != Py_None) { //weights are given
            if (PyObject_Length(weights) != number_of_poles) {
                Standard_Failure::Raise("number of poles and weights mismatch");
                return 0;
            } //complain about mismatch
            Py::Sequence weightssq(weights);
            Standard_Integer index = 1;
            for (Py::Sequence::iterator it = weightssq.begin(); it != weightssq.end(); ++it) {
                Py::Float weight(*it);
                occweights(index++) = weight;
            }
        }
        else { // weights are 1.0
            for (int i=1; i<=occweights.Length(); i++){
                occweights.SetValue(i,1.0);
            }
        }
        // check if the number of poles matches the sum of mults
        if ((PyObject_IsTrue(periodic) && sum_of_mults != number_of_poles) ||
                (PyObject_Not(periodic) && sum_of_mults - degree -1 != number_of_poles)) {
            Standard_Failure::Raise("number of poles and sum of mults mismatch");
            return(0);
        }

        Handle(Geom2d_BSplineCurve) spline = new Geom2d_BSplineCurve(occpoles,occweights,occknots,occmults,degree,
            PyObject_IsTrue(periodic) ? Standard_True : Standard_False);
        if (!spline.IsNull()) {
            this->getGeom2dBSplineCurvePtr()->setHandle(spline);
            Py_Return;
        }
        else {
            Standard_Failure::Raise("failed to create spline");
            return 0; // goes to the catch block
        }
    }
    catch (const Standard_Failure& e) {
        Standard_CString msg = e.GetMessageString();
        PyErr_SetString(PartExceptionOCCError, msg  ? msg : "");
        return 0;
    }
}

PyObject* BSplineCurve2dPy::toBezier(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle(Geom2d_BSplineCurve) spline = Handle(Geom2d_BSplineCurve)::DownCast
        (this->getGeom2dBSplineCurvePtr()->handle());
    Geom2dConvert_BSplineCurveToBezierCurve crt(spline);

    Py::List list;
    Standard_Integer arcs = crt.NbArcs();
    for (Standard_Integer i=1; i<=arcs; i++) {
        Handle(Geom2d_BezierCurve) bezier = crt.Arc(i);
        list.append(Py::asObject(new BezierCurve2dPy(new Geom2dBezierCurve(bezier))));
    }

    return Py::new_reference_to(list);
}

PyObject* BSplineCurve2dPy::join(PyObject *args)
{
    PyObject* c;
    if (!PyArg_ParseTuple(args, "O!", &BSplineCurve2dPy::Type, &c))
        return 0;

    Geom2dBSplineCurve* curve1 = this->getGeom2dBSplineCurvePtr();
    BSplineCurve2dPy* curve2 = static_cast<BSplineCurve2dPy*>(c);
    Handle(Geom2d_BSplineCurve) spline = Handle(Geom2d_BSplineCurve)::DownCast
        (curve2->getGeom2dBSplineCurvePtr()->handle());

    bool ok = curve1->join(spline);

    return PyBool_FromLong(ok ? 1 : 0);
}

PyObject* BSplineCurve2dPy::makeC1Continuous(PyObject *args)
{
    double tol = Precision::Approximation();
    if (!PyArg_ParseTuple(args, "|d", &tol))
        return 0;

    try {
        Geom2dBSplineCurve* spline = this->getGeom2dBSplineCurvePtr();
        spline->makeC1Continuous(tol);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        std::string err = e.GetMessageString();
        if (err.empty()) err = e.DynamicType()->Name();
        PyErr_SetString(PartExceptionOCCError, err.c_str());
        return 0;
    }
}

PyObject* BSplineCurve2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int BSplineCurve2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
