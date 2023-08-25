/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <cmath>
# include <Geom_BSplineCurve.hxx>
# include <GeomAPI_Interpolate.hxx>
# include <GeomAPI_PointsToBSpline.hxx>
# include <GeomConvert_BSplineCurveToBezierCurve.hxx>
# include <gp_Pnt.hxx>
# include <Precision.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColgp_Array1OfVec.hxx>
# include <TColgp_HArray1OfPnt.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_HArray1OfBoolean.hxx>
# include <TColStd_HArray1OfReal.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "BSplineCurvePy.h"
#include "BSplineCurvePy.cpp"
#include "BezierCurvePy.h"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BSplineCurvePy::representation() const
{
    return "<BSplineCurve object>";
}

PyObject *BSplineCurvePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BSplineCurvePy and the Twin object
    return new BSplineCurvePy(new GeomBSplineCurve);
}

// constructor method
int BSplineCurvePy::PyInit(PyObject* args, PyObject* kwd)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    PyObject* obj;
    // poles, [ periodic, degree, interpolate ]
    // {"poles", "mults", "knots", "periodic", "degree", "weights", "CheckRational", NULL};
    obj = buildFromPolesMultsKnots(args, kwd);

    if (obj) {
        Py_DECREF(obj);
        return 0;
    }
    else if (PyErr_ExceptionMatches(PartExceptionOCCError)) {
        return -1;
    }

    PyErr_SetString(PyExc_TypeError, "B-spline constructor accepts:\n"
        "-- poles, [ periodic, degree, interpolate ]\n"
        "-- empty parameter list\n");
    return -1;
}

PyObject*  BSplineCurvePy::__reduce__(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::Tuple tuple(2);

    // type object to create an instance
    Py::Object type(Base::getTypeAsObject(&BSplineCurvePy::Type));
    tuple.setItem(0, type);

    // create an argument tuple to create a copy
    Py::Object self(this);
    Py::Tuple data(7);
    data.setItem(0, Py::Callable(self.getAttr("getPoles")).apply());
    data.setItem(1, Py::Callable(self.getAttr("getMultiplicities")).apply());
    data.setItem(2, Py::Callable(self.getAttr("getKnots")).apply());
    data.setItem(3, Py::Callable(self.getAttr("isPeriodic")).apply());
    data.setItem(4, self.getAttr("Degree"));
    data.setItem(5, Py::Callable(self.getAttr("getWeights")).apply());
    data.setItem(6, Py::Callable(self.getAttr("isRational")).apply());
    tuple.setItem(1, data);

    return Py::new_reference_to(tuple);
}

PyObject* BSplineCurvePy::isRational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = curve->IsRational();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineCurvePy::isPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = curve->IsPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineCurvePy::isClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = curve->IsClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineCurvePy::increaseDegree(PyObject * args)
{
    int degree;
    if (!PyArg_ParseTuple(args, "i", &degree))
        return nullptr;
    PY_TRY {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->IncreaseDegree(degree);
        Py_Return;
    } PY_CATCH_OCC;
}

PyObject* BSplineCurvePy::increaseMultiplicity(PyObject * args)
{
    int mult=-1;
    int start, end;
    if (!PyArg_ParseTuple(args, "ii|i", &start, &end, &mult))
        return nullptr;

    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
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
        return nullptr;
    }
}

PyObject* BSplineCurvePy::incrementMultiplicity(PyObject * args)
{
    int start, end, mult;
    if (!PyArg_ParseTuple(args, "iii", &start, &end, &mult))
        return nullptr;

    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->IncrementMultiplicity(start, end, mult);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

PyObject* BSplineCurvePy::insertKnot(PyObject * args)
{
    double U, tol = 0.0;
    int M=1;
    PyObject* add = Py_True;
    if (!PyArg_ParseTuple(args, "d|idO!", &U, &M, &tol, &PyBool_Type, &add))
        return nullptr;

    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->InsertKnot(U, M, tol, Base::asBoolean(add));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

PyObject* BSplineCurvePy::insertKnots(PyObject * args)
{
    double tol = 0.0;
    PyObject* add = Py_True;
    PyObject* obj1;
    PyObject* obj2;
    if (!PyArg_ParseTuple(args, "OO|dO!", &obj1,
                                          &obj2,
                                          &tol, &PyBool_Type, &add))
        return nullptr;

    try {
        Py::Sequence knots(obj1);
        TColStd_Array1OfReal k(1, knots.size());
        int index=1;
        for (Py::Sequence::iterator it = knots.begin(); it != knots.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }
        Py::Sequence mults(obj2);
        TColStd_Array1OfInteger m(1, mults.size());
        index=1;
        for (Py::Sequence::iterator it = mults.begin(); it != mults.end(); ++it) {
            Py::Long val(*it);
            m(index++) = (int)val;
        }

        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->InsertKnots(k, m, tol, Base::asBoolean(add));
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

PyObject* BSplineCurvePy::removeKnot(PyObject * args)
{
    double tol;
    int Index, M;
    if (!PyArg_ParseTuple(args, "iid", &Index, &M, &tol))
        return nullptr;

    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        Standard_Boolean ok = curve->RemoveKnot(Index, M, tol);
        return PyBool_FromLong(ok ? 1 : 0);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::segment(PyObject * args)
{
    double u1, u2;
    if (!PyArg_ParseTuple(args, "dd", &u1, &u2))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        Handle(Geom_BSplineCurve) tempCurve = Handle(Geom_BSplineCurve)::DownCast
            (curve->Copy());
        tempCurve->Segment(u1, u2);
        if (std::abs(tempCurve->FirstParameter()-u1) > Precision::Approximation() ||
            std::abs(tempCurve->LastParameter()-u2) > Precision::Approximation()) {
            Standard_Failure::Raise("Failed to segment BSpline curve");
            return nullptr;
        }
        else {
            curve->Segment(u1, u2);
        }
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::setKnot(PyObject * args)
{
    int Index, M=-1;
    double K;
    if (!PyArg_ParseTuple(args, "id|i", &Index, &K, &M))
        return nullptr;

    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        if (M == -1) {
            curve->SetKnot(Index, K);
        }
        else {
            curve->SetKnot(Index, K, M);
        }

        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::getKnot(PyObject * args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return nullptr;

    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        double M = curve->Knot(Index);

        return Py_BuildValue("d",M);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::setKnots(PyObject * args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return nullptr;
    try {
        Py::Sequence list(obj);
        TColStd_Array1OfReal k(1,list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }

        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->SetKnots(k);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::getKnots(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
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
        return nullptr;
    }
}

PyObject* BSplineCurvePy::setPole(PyObject * args)
{
    int index;
    double weight=-1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iO!|d", &index, &(Base::VectorPy::Type), &p, &weight))
        return nullptr;
    Base::Vector3d vec = static_cast<Base::VectorPy*>(p)->value();
    gp_Pnt pnt(vec.x, vec.y, vec.z);
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
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

PyObject* BSplineCurvePy::getPole(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        Standard_OutOfRange_Raise_if
            (index < 1 || index > curve->NbPoles(), "Pole index out of range");
        gp_Pnt pnt = curve->Pole(index);
        Base::VectorPy* vec = new Base::VectorPy(Base::Vector3d(
            pnt.X(), pnt.Y(), pnt.Z()));
        return vec;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::getPoles(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        TColgp_Array1OfPnt p(1,curve->NbPoles());
        curve->Poles(p);
        Py::List poles;
        for (Standard_Integer i=p.Lower(); i<=p.Upper(); i++) {
            gp_Pnt pnt = p(i);
            Base::VectorPy* vec = new Base::VectorPy(Base::Vector3d(
                pnt.X(), pnt.Y(), pnt.Z()));
            poles.append(Py::asObject(vec));
        }
        return Py::new_reference_to(poles);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::getPolesAndWeights(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        TColgp_Array1OfPnt p(1,curve->NbPoles());
        curve->Poles(p);
        TColStd_Array1OfReal w(1,curve->NbPoles());
        curve->Weights(w);

        Py::List poles;
        for (Standard_Integer i=p.Lower(); i<=p.Upper(); i++) {
            gp_Pnt pnt = p(i);
            double weight = w(i);
            Py::Tuple t(4);
            t.setItem(0, Py::Float(pnt.X()));
            t.setItem(1, Py::Float(pnt.Y()));
            t.setItem(2, Py::Float(pnt.Z()));
            t.setItem(3, Py::Float(weight));
            poles.append(t);
        }
        return Py::new_reference_to(poles);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::setWeight(PyObject * args)
{
    int index;
    double weight;
    if (!PyArg_ParseTuple(args, "id", &index,&weight))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->SetWeight(index,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::getWeight(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
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

PyObject* BSplineCurvePy::getWeights(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
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

PyObject* BSplineCurvePy::getResolution(PyObject * args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        double utol;
        curve->Resolution(tol,utol);
        return Py_BuildValue("d",utol);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::movePoint(PyObject * args)
{
    double U;
    int index1, index2;
    PyObject* pnt;
    if (!PyArg_ParseTuple(args, "dO!ii", &U, &(Base::VectorPy::Type),&pnt, &index1, &index2))
        return nullptr;
    try {
        Base::Vector3d p = static_cast<Base::VectorPy*>(pnt)->value();
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        int first, last;
        curve->MovePoint(U, gp_Pnt(p.x,p.y,p.z), index1, index2, first, last);
        return Py_BuildValue("(ii)",first, last);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::setNotPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->SetNotPeriodic();
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::setPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->SetPeriodic();
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::setOrigin(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->SetOrigin(index);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::getMultiplicity(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        int mult = curve->Multiplicity(index);
        return Py_BuildValue("i", mult);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::getMultiplicities(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfInteger m(1,curve->NbKnots());
        curve->Multiplicities(m);
        Py::List mults;
        for (Standard_Integer i=m.Lower(); i<=m.Upper(); i++) {
            mults.append(Py::Long(m(i)));
        }
        return Py::new_reference_to(mults);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}
Py::Long BSplineCurvePy::getDegree() const
{
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(curve->Degree());
}

Py::Long BSplineCurvePy::getMaxDegree() const
{
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(curve->MaxDegree());
}

Py::Long BSplineCurvePy::getNbPoles() const
{
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(curve->NbPoles());
}

Py::Long BSplineCurvePy::getNbKnots() const
{
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(curve->NbKnots());
}

Py::Object BSplineCurvePy::getStartPoint() const
{
    Handle(Geom_BSplineCurve) c = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt pnt = c->StartPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

Py::Object BSplineCurvePy::getEndPoint() const
{
    Handle(Geom_BSplineCurve) c = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt pnt = c->EndPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

Py::Object BSplineCurvePy::getFirstUKnotIndex() const
{
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(curve->FirstUKnotIndex());
}

Py::Object BSplineCurvePy::getLastUKnotIndex() const
{
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(curve->LastUKnotIndex());
}

Py::List BSplineCurvePy::getKnotSequence() const
{
    Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast
        (getGeometryPtr()->handle());
    Standard_Integer m = 0;
    if (curve->IsPeriodic()) {
        // knots=poles+2*degree-mult(1)+2
        m = curve->NbPoles() + 2*curve->Degree() - curve->Multiplicity(1) + 2;
    }
    else {
        // knots=poles+degree+1
        for (int i=1; i<= curve->NbKnots(); i++)
            m += curve->Multiplicity(i);
    }

    TColStd_Array1OfReal k(1,m);
    curve->KnotSequence(k);
    Py::List list;
    for (Standard_Integer i=k.Lower(); i<=k.Upper(); i++) {
        list.append(Py::Float(k(i)));
    }
    return list;
}

PyObject* BSplineCurvePy::toBiArcs(PyObject * args)
{
    double tolerance = 0.001;
    if (!PyArg_ParseTuple(args, "d", &tolerance))
        return nullptr;
    try {
        GeomBSplineCurve* curve = getGeomBSplineCurvePtr();
        std::list<Geometry*> arcs;
        arcs = curve->toBiArcs(tolerance);

        Py::List list;
        for (auto arc : arcs) {
            list.append(Py::asObject(arc->getPyObject()));
            delete arc;
        }

        return Py::new_reference_to(list);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::approximate(PyObject *args, PyObject *kwds)
{
    PyObject *obj;
    Standard_Integer degMin = 3;
    Standard_Integer degMax = 8;
    Standard_Integer segMax = 8;
    char *continuity = "C2";
    double tol3d = 1e-3;
    char *parType = "ChordLength";
    PyObject *par = nullptr;
    double weight1 = 0;
    double weight2 = 0;
    double weight3 = 0;

    // Approximate this curve with a given continuity and degree
    static const std::array<const char *, 5> kwds_reapprox{"MaxDegree", "MaxSegments", "Continuity", "Tolerance",
                                                           nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "i|isd", kwds_reapprox,
                                            &tol3d, &degMax, &segMax, &continuity)) {

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

        bool ok = this->getGeomBSplineCurvePtr()->approximate(tol3d, segMax, degMax, c);
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }

    // Approximate a list of points
    //
    static const std::array<const char *, 11> kwds_interp{"Points", "DegMax", "Continuity", "Tolerance", "DegMin",
                                                          "ParamType", "Parameters", "LengthWeight", "CurvatureWeight",
                                                          "TorsionWeight", nullptr};

    PyErr_Clear();
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O|isdisOddd", kwds_interp,
                                             &obj, &degMax,
                                             &continuity, &tol3d, &degMin,
                                             &parType, &par,
                                             &weight1, &weight2, &weight3)) {
        return nullptr;
    }

    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt pnts(1, list.size());
        Standard_Integer index = 1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Base::Vector3d vec = Py::Vector(*it).toVector();
            pnts(index++) = gp_Pnt(vec.x,vec.y,vec.z);
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
            GeomAPI_PointsToBSpline fit(pnts, weight1, weight2, weight3, degMax, c, tol3d);
            Handle(Geom_BSplineCurve) spline = fit.Curve();
            if (!spline.IsNull()) {
                this->getGeomBSplineCurvePtr()->setHandle(spline);
                Py_Return;
            }
            else {
                Standard_Failure::Raise("Smoothing approximation failed");
                return nullptr; // goes to the catch block
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

            GeomAPI_PointsToBSpline fit(pnts, parameters, degMin, degMax, c, tol3d);
            Handle(Geom_BSplineCurve) spline = fit.Curve();
            if (!spline.IsNull()) {
                this->getGeomBSplineCurvePtr()->setHandle(spline);
                Py_Return;
            }
            else {
                Standard_Failure::Raise("Approximation with parameters failed");
                return nullptr; // goes to the catch block
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

        GeomAPI_PointsToBSpline fit(pnts, pt, degMin, degMax, c, tol3d);
        Handle(Geom_BSplineCurve) spline = fit.Curve();
        if (!spline.IsNull()) {
            this->getGeomBSplineCurvePtr()->setHandle(spline);
            Py_Return;
        }
        else {
            Standard_Failure::Raise("failed to approximate points");
            return nullptr; // goes to the catch block
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::getCardinalSplineTangents(PyObject *args, PyObject *kwds)
{
    PyObject* pts;
    PyObject* tgs;
    double parameter;

    static const std::array<const char *, 3> kwds_interp1 {"Points", "Parameter", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "Od",kwds_interp1, &pts, &parameter)) {
        Py::Sequence list(pts);
        std::vector<gp_Pnt> interpPoints;
        interpPoints.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            interpPoints.emplace_back(pnt.x,pnt.y,pnt.z);
        }

        GeomBSplineCurve* bspline = this->getGeomBSplineCurvePtr();
        std::vector<gp_Vec> tangents;
        bspline->getCardinalSplineTangents(interpPoints, parameter, tangents);

        Py::List vec;
        for (gp_Vec it : tangents)
            vec.append(Py::Vector(Base::Vector3d(it.X(), it.Y(), it.Z())));
        return Py::new_reference_to(vec);
    }

    PyErr_Clear();
    static const std::array<const char *, 3> kwds_interp2 {"Points", "Parameters", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "OO",kwds_interp2, &pts, &tgs)) {
        Py::Sequence list(pts);
        std::vector<gp_Pnt> interpPoints;
        interpPoints.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            interpPoints.emplace_back(pnt.x,pnt.y,pnt.z);
        }

        Py::Sequence list2(tgs);
        std::vector<double> parameters;
        parameters.reserve(list2.size());
        for (Py::Sequence::iterator it = list2.begin(); it != list2.end(); ++it) {
            Py::Float p(*it);
            parameters.push_back(static_cast<double>(p));
        }

        GeomBSplineCurve* bspline = this->getGeomBSplineCurvePtr();
        std::vector<gp_Vec> tangents;
        bspline->getCardinalSplineTangents(interpPoints, parameters, tangents);

        Py::List vec;
        for (gp_Vec it : tangents)
            vec.append(Py::Vector(Base::Vector3d(it.X(), it.Y(), it.Z())));
        return Py::new_reference_to(vec);
    }

    return nullptr;
}

PyObject* BSplineCurvePy::interpolate(PyObject *args, PyObject *kwds)
{
    PyObject* obj;
    PyObject* par = nullptr;
    double tol3d = Precision::Approximation();
    PyObject* periodic = Py_False;
    PyObject* t1 = nullptr; PyObject* t2 = nullptr;
    PyObject* ts = nullptr; PyObject* fl = nullptr;
    PyObject* scale = Py_True;

    static const std::array<const char *, 10> kwds_interp{"Points", "PeriodicFlag", "Tolerance", "InitialTangent",
                                                          "FinalTangent", "Tangents", "TangentFlags", "Parameters",
                                                          "Scale", nullptr};

    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O|O!dO!O!OOOO!", kwds_interp,
                                             &obj, &PyBool_Type, &periodic, &tol3d,
                                             &Base::VectorPy::Type, &t1,
                                             &Base::VectorPy::Type, &t2,
                                             &ts, &fl, &par, &PyBool_Type, &scale)) {
        return nullptr;
    }

    try {
        Py::Sequence list(obj);
        Handle(TColgp_HArray1OfPnt) interpolationPoints = new TColgp_HArray1OfPnt(1, list.size());
        Standard_Integer index = 1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            interpolationPoints->SetValue(index++, gp_Pnt(pnt.x,pnt.y,pnt.z));
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

        std::unique_ptr<GeomAPI_Interpolate> aBSplineInterpolation;
        if (parameters.IsNull()) {
            aBSplineInterpolation = std::make_unique<GeomAPI_Interpolate>(interpolationPoints,
                Base::asBoolean(periodic), tol3d);
        }
        else {
            aBSplineInterpolation = std::make_unique<GeomAPI_Interpolate>(interpolationPoints, parameters,
                Base::asBoolean(periodic), tol3d);
        }

        if (t1 && t2) {
            Base::Vector3d v1 = Py::Vector(t1,false).toVector();
            Base::Vector3d v2 = Py::Vector(t2,false).toVector();
            gp_Vec initTangent(v1.x,v1.y,v1.z), finalTangent(v2.x,v2.y,v2.z);
            aBSplineInterpolation->Load(initTangent, finalTangent, Base::asBoolean(scale));
        }
        else if (ts && fl) {
            Py::Sequence tlist(ts);
            TColgp_Array1OfVec tangents(1, tlist.size());
            Standard_Integer index = 1;
            for (Py::Sequence::iterator it = tlist.begin(); it != tlist.end(); ++it) {
                Py::Vector v(*it);
                Base::Vector3d vec = v.toVector();
                tangents.SetValue(index++, gp_Vec(vec.x,vec.y,vec.z));
            }

            Py::Sequence flist(fl);
            Handle(TColStd_HArray1OfBoolean) tangentFlags = new TColStd_HArray1OfBoolean(1, flist.size());
            Standard_Integer findex = 1;
            for (Py::Sequence::iterator it = flist.begin(); it != flist.end(); ++it) {
                Py::Boolean flag(*it);
                tangentFlags->SetValue(findex++, static_cast<bool>(flag) ? Standard_True : Standard_False);
            }

            aBSplineInterpolation->Load(tangents, tangentFlags, Base::asBoolean(scale));
        }

        aBSplineInterpolation->Perform();
        if (aBSplineInterpolation->IsDone()) {
            Handle(Geom_BSplineCurve) aBSplineCurve(aBSplineInterpolation->Curve());
            this->getGeomBSplineCurvePtr()->setHandle(aBSplineCurve);
            Py_Return;
        }
        else {
            Standard_Failure::Raise("failed to interpolate points");
            return nullptr; // goes to the catch block
        }
    }
    catch (Standard_Failure& e) {
        std::string err = e.GetMessageString();
        if (err.empty()) err = e.DynamicType()->Name();
        PyErr_SetString(PartExceptionOCCError, err.c_str());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::buildFromPoles(PyObject *args)
{
    PyObject* obj;
    int degree = 3;
    PyObject* periodic = Py_False;
    PyObject* interpolate = Py_False;
    if (!PyArg_ParseTuple(args, "O|O!iO!",&obj, &PyBool_Type, &periodic, &degree, &PyBool_Type, interpolate))
        return nullptr;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        Standard_Integer index = 1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            poles(index++) = gp_Pnt(pnt.x,pnt.y,pnt.z);
        }

        if (poles.Length() <= degree)
            degree = poles.Length()-1;

        if (Base::asBoolean(periodic)) {
            int mult;
            int len;
            if (Base::asBoolean(interpolate)) {
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

            Handle(Geom_BSplineCurve) spline = new Geom_BSplineCurve(poles, knots, mults, degree, Standard_True);
            if (!spline.IsNull()) {
                this->getGeomBSplineCurvePtr()->setHandle(spline);
                Py_Return;
            }
            else {
                Standard_Failure::Raise("failed to create spline");
                return nullptr; // goes to the catch block
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

            Handle(Geom_BSplineCurve) spline = new Geom_BSplineCurve(poles, knots, mults, degree, Standard_False);
            if (!spline.IsNull()) {
                this->getGeomBSplineCurvePtr()->setHandle(spline);
                Py_Return;
            }
            else {
                Standard_Failure::Raise("failed to create spline");
                return nullptr; // goes to the catch block
            }
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::buildFromPolesMultsKnots(PyObject *args, PyObject *keywds)
{
    static const std::array<const char *, 8> kwlist{"poles", "mults", "knots", "periodic", "degree", "weights",
                                                    "CheckRational", nullptr};
    PyObject* periodic = Py_False; // NOLINT
    PyObject* CheckRational = Py_True; // NOLINT
    PyObject* poles = Py_None;
    PyObject* mults = Py_None;
    PyObject* knots = Py_None;
    PyObject* weights = Py_None;
    int degree = 3;
    int number_of_poles = 0;
    int number_of_knots = 0;
    int sum_of_mults = 0;
    if (!Base::Wrapped_ParseTupleAndKeywords(args, keywds, "O|OOO!iOO!", kwlist,
                                             &poles, &mults, &knots,
                                             &PyBool_Type, &periodic,
                                             &degree, &weights,
                                             &PyBool_Type, &CheckRational)) {
        return nullptr;
    }
    try {
        // poles have to be present
        Py::Sequence list(poles);

        number_of_poles = list.size();
        if ((number_of_poles) < 2) {
            Standard_Failure::Raise("need two or more poles");
            return nullptr;
        }
        TColgp_Array1OfPnt occpoles(1, number_of_poles);
        Standard_Integer index = 1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            occpoles(index++) = gp_Pnt(pnt.x,pnt.y,pnt.z);
        }
        //Calculate the number of knots
        if (mults != Py_None && knots != Py_None) {
            number_of_knots = PyObject_Length(mults);
            if (PyObject_Length(knots) != number_of_knots) {
                Standard_Failure::Raise("number of knots and mults mismatch");
                return nullptr;
            }
        }
        else {
            if (mults != Py_None) {
                number_of_knots = PyObject_Length(mults);
            }
            else {
                if (knots != Py_None) { number_of_knots = PyObject_Length(knots); }
                else { //guess number of knots
                    if (Base::asBoolean(periodic)) {
                        if (number_of_poles < degree) {degree = number_of_poles;}
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
                Py::Long mult(*it);
                if (index < occmults.Length() || !Base::asBoolean(periodic)) {
                    sum_of_mults += static_cast<int>(mult); //sum up the mults to compare them against the number of poles later
                }
                occmults(index++) = static_cast<int>(mult);
            }
        }
        else { //mults are 1 or degree+1 at the ends
            for (int i=1; i<=occmults.Length(); i++){
                occmults.SetValue(i,1);
            }
            if (!Base::asBoolean(periodic) && occmults.Length() > 0) {
                occmults.SetValue(1, degree+1);
                occmults.SetValue(occmults.Length(), degree+1);
                sum_of_mults = occmults.Length()+2*degree;
            }
            else {
                sum_of_mults = occmults.Length()-1;
            }
        }
        // check multiplicity of inner knots
        for (Standard_Integer i=2; i < occmults.Length(); i++) {
            if (occmults(i) > degree) {
                Standard_Failure::Raise("multiplicity of inner knot higher than degree");
            }
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
                return nullptr;
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
        if ((Base::asBoolean(periodic) && sum_of_mults != number_of_poles) ||
            (!Base::asBoolean(periodic) && sum_of_mults - degree -1 != number_of_poles)) {
            Standard_Failure::Raise("number of poles and sum of mults mismatch");
            return(nullptr);
        }

        Handle(Geom_BSplineCurve) spline = new Geom_BSplineCurve(occpoles,occweights,occknots,occmults,degree,
            Base::asBoolean(periodic), Base::asBoolean(CheckRational));
        if (!spline.IsNull()) {
            this->getGeomBSplineCurvePtr()->setHandle(spline);
            Py_Return;
        }
        else {
            Standard_Failure::Raise("failed to create spline");
            return nullptr; // goes to the catch block
        }
    }
    catch (const Standard_Failure& e) {
        Standard_CString msg = e.GetMessageString();
        PyErr_SetString(PartExceptionOCCError, msg  ? msg : "");
        return nullptr;
    }
}


PyObject* BSplineCurvePy::toBezier(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(Geom_BSplineCurve) spline = Handle(Geom_BSplineCurve)::DownCast
            (this->getGeomBSplineCurvePtr()->handle());
        GeomConvert_BSplineCurveToBezierCurve crt(spline);

        Py::List list;
        Standard_Integer arcs = crt.NbArcs();
        for (Standard_Integer i=1; i<=arcs; i++) {
            Handle(Geom_BezierCurve) bezier = crt.Arc(i);
            list.append(Py::asObject(new BezierCurvePy(new GeomBezierCurve(bezier))));
        }

        return Py::new_reference_to(list);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::join(PyObject *args)
{
    PyObject* c;
    if (!PyArg_ParseTuple(args, "O!", &BSplineCurvePy::Type, &c))
        return nullptr;

    try {
        GeomBSplineCurve* curve1 = this->getGeomBSplineCurvePtr();
        BSplineCurvePy* curve2 = static_cast<BSplineCurvePy*>(c);
        Handle(Geom_BSplineCurve) spline = Handle(Geom_BSplineCurve)::DownCast
            (curve2->getGeomBSplineCurvePtr()->handle());

        bool ok = curve1->join(spline);

        return PyBool_FromLong(ok ? 1 : 0);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::makeC1Continuous(PyObject *args)
{
    double tol = Precision::Approximation();
    double ang_tol = 1.0e-7;
    if (!PyArg_ParseTuple(args, "|dd", &tol, &ang_tol))
        return nullptr;

    try {
        GeomBSplineCurve* spline = this->getGeomBSplineCurvePtr();
        spline->makeC1Continuous(tol, ang_tol);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        std::string err = e.GetMessageString();
        if (err.empty()) err = e.DynamicType()->Name();
        PyErr_SetString(PartExceptionOCCError, err.c_str());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::scaleKnotsToBounds(PyObject *args)
{
    double u0=0.0;
    double u1=1.0;
    if (!PyArg_ParseTuple(args, "|dd", &u0, &u1))
        return nullptr;
    try {
        if (u0 >= u1) {
            Standard_Failure::Raise("Bad parameter range");
            return nullptr;
        }
        GeomBSplineCurve* curve = getGeomBSplineCurvePtr();
        curve->scaleKnotsToBounds(u0, u1);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        std::string err = e.GetMessageString();
        if (err.empty()) err = e.DynamicType()->Name();
        PyErr_SetString(PartExceptionOCCError, err.c_str());
        return nullptr;
    }
}

PyObject* BSplineCurvePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BSplineCurvePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
