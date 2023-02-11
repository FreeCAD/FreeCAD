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
# include <BSplCLib.hxx>
# include <Geom_BezierCurve.hxx>
# include <gp_Pnt.hxx>
# include <math_Gauss.hxx>
# include <math_Matrix.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColStd_Array1OfReal.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "BezierCurvePy.h"
#include "BezierCurvePy.cpp"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BezierCurvePy::representation() const
{
    return "<BezierCurve object>";
}

PyObject *BezierCurvePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BezierCurvePy and the Twin object
    return new BezierCurvePy(new GeomBezierCurve);
}

// constructor method
int BezierCurvePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* BezierCurvePy::isRational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = curve->IsRational();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierCurvePy::isPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = curve->IsPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierCurvePy::isClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = curve->IsClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierCurvePy::increase(PyObject * args)
{
    int degree;
    if (!PyArg_ParseTuple(args, "i", &degree))
        return nullptr;
    Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
    curve->Increase(degree);
    Py_Return;
}

PyObject* BezierCurvePy::insertPoleAfter(PyObject * args)
{
    int index;
    double weight=1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iO!|d", &index, &(Base::VectorPy::Type), &p, &weight))
        return nullptr;
    Base::Vector3d vec = static_cast<Base::VectorPy*>(p)->value();
    gp_Pnt pnt(vec.x, vec.y, vec.z);
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->InsertPoleAfter(index,pnt,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurvePy::insertPoleBefore(PyObject * args)
{
    int index;
    double weight=1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iO!|d", &index, &(Base::VectorPy::Type), &p, &weight))
        return nullptr;
    Base::Vector3d vec = static_cast<Base::VectorPy*>(p)->value();
    gp_Pnt pnt(vec.x, vec.y, vec.z);
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->InsertPoleBefore(index,pnt,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurvePy::removePole(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->RemovePole(index);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurvePy::segment(PyObject * args)
{
    double u1,u2;
    if (!PyArg_ParseTuple(args, "dd", &u1,&u2))
        return nullptr;
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->Segment(u1,u2);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurvePy::setPole(PyObject * args)
{
    int index;
    double weight=-1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iO!|d", &index, &(Base::VectorPy::Type), &p, &weight))
        return nullptr;
    Base::Vector3d vec = static_cast<Base::VectorPy*>(p)->value();
    gp_Pnt pnt(vec.x, vec.y, vec.z);
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
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

PyObject* BezierCurvePy::getPole(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
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

PyObject* BezierCurvePy::getPoles(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
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

PyObject* BezierCurvePy::setPoles(PyObject * args)
{
    PyObject* plist;
    if (!PyArg_ParseTuple(args, "O", &plist))
        return nullptr;
    try {
        Py::Sequence list(plist);
        TColgp_Array1OfPnt poles(1,list.size());
        int index = poles.Lower();
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pole = v.toVector();
            poles.SetValue(index++, gp_Pnt(pole.x,pole.y,pole.z));
        }

        Handle(Geom_BezierCurve) bezier = new Geom_BezierCurve(poles);
        this->getGeomBezierCurvePtr()->setHandle(bezier);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurvePy::setWeight(PyObject * args)
{
    int index;
    double weight;
    if (!PyArg_ParseTuple(args, "id", &index,&weight))
        return nullptr;
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
            (getGeometryPtr()->handle());
        curve->SetWeight(index,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierCurvePy::getWeight(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
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

PyObject* BezierCurvePy::getWeights(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
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

PyObject* BezierCurvePy::getResolution(PyObject* args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return nullptr;
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
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
Py::Long BezierCurvePy::getDegree() const
{
    Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(curve->Degree());
}

Py::Long BezierCurvePy::getMaxDegree() const
{
    Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(curve->MaxDegree());
}

Py::Long BezierCurvePy::getNbPoles() const
{
    Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(curve->NbPoles());
}

Py::Object BezierCurvePy::getStartPoint() const
{
    Handle(Geom_BezierCurve) c = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt pnt = c->StartPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

Py::Object BezierCurvePy::getEndPoint() const
{
    Handle(Geom_BezierCurve) c = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt pnt = c->EndPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

PyObject* BezierCurvePy::interpolate(PyObject * args)
{
    PyObject* obj;
    PyObject* par=nullptr;
    if (!PyArg_ParseTuple(args, "O|O", &obj, &par))
        return nullptr;
    try {
        Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast
        (getGeometryPtr()->handle());
        Py::Sequence constraints(obj);
        int nb_pts = constraints.size();
        if (nb_pts < 2)
            Standard_Failure::Raise("not enough points given");

        TColStd_Array1OfReal params(1, nb_pts);
        if (par) {
            Py::Sequence plist(par);
            int param_size = plist.size();
            if (param_size != nb_pts)
                Standard_Failure::Raise("number of points and parameters don't match");
            int idx=1;
            for (Py::Sequence::iterator pit = plist.begin(); pit != plist.end(); ++pit) {
                Py::Float val(*pit);
                params(idx++) = (double)val;
            }
        }
        else {
            for (int idx=0; idx<nb_pts; ++idx) {
                params(idx+1) = (double)idx/((double)nb_pts-1);
            }
        }

        int num_poles = 0;
        for (Py::Sequence::iterator it1 = constraints.begin(); it1 != constraints.end(); ++it1) {
            Py::Sequence row(*it1);
            num_poles += (int)row.size();
        }
        if (num_poles > curve->MaxDegree())
            Standard_Failure::Raise("number of constraints exceeds bezier curve capacity");
        // create a bezier-type knot sequence
        TColStd_Array1OfReal knots(1, 2*num_poles);
        for (int idx=1; idx<=num_poles; ++idx) {
            knots(idx) = params(1);
            knots(num_poles+idx) = params(nb_pts);
        }
        math_Matrix OCCmatrix(1, num_poles, 1, num_poles, 0.0);
        math_Vector res_x(1, num_poles, 0.0);
        math_Vector res_y(1, num_poles, 0.0);
        math_Vector res_z(1, num_poles, 0.0);
        int row_idx = 1;
        int cons_idx = 1;
        for (Py::Sequence::iterator it1 = constraints.begin(); it1 != constraints.end(); ++it1) {
            Py::Sequence row(*it1);
            math_Matrix bezier_eval(1, row.size(), 1, num_poles, 0.0);
            Standard_Integer first_non_zero;
            BSplCLib::EvalBsplineBasis(row.size()-1, num_poles, knots, params(cons_idx), first_non_zero, bezier_eval, Standard_False);
            int idx2 = 1;
            for (Py::Sequence::iterator it2 = row.begin(); it2 != row.end(); ++it2) {
                OCCmatrix.SetRow(row_idx, bezier_eval.Row(idx2));
                Py::Vector v(*it2);
                Base::Vector3d pnt = v.toVector();
                res_x(row_idx) = pnt.x;
                res_y(row_idx) = pnt.y;
                res_z(row_idx) = pnt.z;
                idx2++;
                row_idx++;
            }
            cons_idx++;
        }
        math_Gauss gauss(OCCmatrix);
        gauss.Solve(res_x);
        if (!gauss.IsDone())
            Standard_Failure::Raise("Failed to solve equations");
        gauss.Solve(res_y);
        if (!gauss.IsDone())
            Standard_Failure::Raise("Failed to solve equations");
        gauss.Solve(res_z);
        if (!gauss.IsDone())
            Standard_Failure::Raise("Failed to solve equations");

        TColgp_Array1OfPnt poles(1,num_poles);
        for (int idx=1; idx<=num_poles; ++idx) {
            poles.SetValue(idx, gp_Pnt(res_x(idx),res_y(idx),res_z(idx)));
        }

        Handle(Geom_BezierCurve) bezier = new Geom_BezierCurve(poles);
        this->getGeomBezierCurvePtr()->setHandle(bezier);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject *BezierCurvePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BezierCurvePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
