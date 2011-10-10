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
# include <Geom_BSplineCurve.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Handle_Geom_BSplineCurve.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_Array2OfReal.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColgp_Array2OfPnt.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "Geometry.h"
#include "BSplineCurvePy.h"
#include "BSplineSurfacePy.h"
#include "BSplineSurfacePy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BSplineSurfacePy::representation(void) const
{
    return "<BSplineSurface object>";
}

PyObject *BSplineSurfacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BSplineSurfacePy and the Twin object 
    return new BSplineSurfacePy(new GeomBSplineSurface);
}

// constructor method
int BSplineSurfacePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* BSplineSurfacePy::bounds(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    Py::Tuple bound(4);
    Standard_Real u1,u2,v1,v2;
    surf->Bounds(u1,u2,v1,v2);
    bound.setItem(0,Py::Float(u1));
    bound.setItem(1,Py::Float(u2));
    bound.setItem(2,Py::Float(v1));
    bound.setItem(3,Py::Float(v2));
    return Py::new_reference_to(bound);
}

PyObject* BSplineSurfacePy::isURational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsURational();
    if (val) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineSurfacePy::isVRational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVRational();
    if (val) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineSurfacePy::isUPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUPeriodic();
    if (val) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineSurfacePy::isVPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVPeriodic();
    if (val) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineSurfacePy::isUClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUClosed();
    if (val) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineSurfacePy::isVClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVPeriodic();
    if (val) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineSurfacePy::increaseDegree(PyObject *args)
{
    int udegree, vdegree;
    if (!PyArg_ParseTuple(args, "ii",&udegree,&vdegree))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    surf->IncreaseDegree(udegree,vdegree);
    Py_Return;
}

PyObject* BSplineSurfacePy::increaseUMultiplicity(PyObject *args)
{
    int mult=-1;
    int start, end;
    if (!PyArg_ParseTuple(args, "ii|i", &start, &end, &mult))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    if (mult == -1) {
        mult = end;
        surf->IncreaseUMultiplicity(start, mult);
    }
    else {
        surf->IncreaseUMultiplicity(start, end, mult);
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::increaseVMultiplicity(PyObject *args)
{
    int mult=-1;
    int start, end;
    if (!PyArg_ParseTuple(args, "ii|i", &start, &end, &mult))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    if (mult == -1) {
        mult = end;
        surf->IncreaseVMultiplicity(start, mult);
    }
    else {
        surf->IncreaseVMultiplicity(start, end, mult);
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::incrementUMultiplicity(PyObject *args)
{
    int start, end, mult;
    if (!PyArg_ParseTuple(args, "iii", &start, &end, &mult))
        return 0;

    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->IncrementUMultiplicity(start, end, mult);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::incrementVMultiplicity(PyObject *args)
{
    int start, end, mult;
    if (!PyArg_ParseTuple(args, "iii", &start, &end, &mult))
        return 0;

    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->IncrementVMultiplicity(start, end, mult);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::insertUKnot(PyObject *args)
{
    double U, tol = 0.0;
    int M=1;
    PyObject* add = Py_True;
    if (!PyArg_ParseTuple(args, "did|O!", &U, &M, &tol, &PyBool_Type, &add))
        return 0;

    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->InsertUKnot(U,M,tol,(add==Py_True));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::insertUKnots(PyObject *args)
{
    double tol = 0.0;
    PyObject* add = Py_True;
    PyObject* obj1;
    PyObject* obj2;
    if (!PyArg_ParseTuple(args, "O!O!|dO!", &PyList_Type, &obj1,
                                            &PyList_Type, &obj2,
                                            &tol, &PyBool_Type, &add))
        return 0;

    try {
        Py::List knots(obj1);
        TColStd_Array1OfReal k(1,knots.size());
        int index=1;
        for (Py::List::iterator it = knots.begin(); it != knots.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }
        Py::List mults(obj2);
        TColStd_Array1OfInteger m(1,mults.size());
        index=1;
        for (Py::List::iterator it = mults.begin(); it != mults.end(); ++it) {
            Py::Int val(*it);
            m(index++) = (int)val;
        }

        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->InsertUKnots(k,m,tol,(add==Py_True));
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::insertVKnot(PyObject *args)
{
    double V, tol = 0.0;
    int M=1;
    PyObject* add = Py_True;
    if (!PyArg_ParseTuple(args, "did|O!", &V, &M, &tol, &PyBool_Type, &add))
        return 0;

    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->InsertVKnot(V,M,tol,(add==Py_True));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::insertVKnots(PyObject *args)
{
    double tol = 0.0;
    PyObject* add = Py_True;
    PyObject* obj1;
    PyObject* obj2;
    if (!PyArg_ParseTuple(args, "O!O!|dO!", &PyList_Type, &obj1,
                                            &PyList_Type, &obj2,
                                            &tol, &PyBool_Type, &add))
        return 0;

    try {
        Py::List knots(obj1);
        TColStd_Array1OfReal k(1,knots.size());
        int index=1;
        for (Py::List::iterator it = knots.begin(); it != knots.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }
        Py::List mults(obj2);
        TColStd_Array1OfInteger m(1,mults.size());
        index=1;
        for (Py::List::iterator it = mults.begin(); it != mults.end(); ++it) {
            Py::Int val(*it);
            m(index++) = (int)val;
        }

        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->InsertUKnots(k,m,tol,(add==Py_True));
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::removeUKnot(PyObject *args)
{
    double tol;
    int Index,M;
    if (!PyArg_ParseTuple(args, "iid", &Index, &M, &tol))
        return 0;

    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        Standard_Boolean ok = surf->RemoveUKnot(Index,M,tol);
        if (ok) {
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

PyObject* BSplineSurfacePy::removeVKnot(PyObject *args)
{
    double tol;
    int Index,M;
    if (!PyArg_ParseTuple(args, "iid", &Index, &M, &tol))
        return 0;

    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        Standard_Boolean ok = surf->RemoveVKnot(Index,M,tol);
        if (ok) {
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

PyObject* BSplineSurfacePy::segment(PyObject *args)
{
    double u1,u2,v1,v2;
    if (!PyArg_ParseTuple(args, "dddd", &u1,&u2,&v1,&v2))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->Segment(u1,u2,v1,v2);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setUKnot(PyObject *args)
{
    int Index, M=-1;
    double K;
    if (!PyArg_ParseTuple(args, "id|i", &Index, &K, &M))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    if (M == -1) {
        surf->SetUKnot(Index, K);
    }
    else {
        surf->SetUKnot(Index, K, M);
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::setVKnot(PyObject *args)
{
    int Index, M=-1;
    double K;
    if (!PyArg_ParseTuple(args, "id|i", &Index, &K, &M))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    if (M == -1) {
        surf->SetUKnot(Index, K);
    }
    else {
        surf->SetUKnot(Index, K, M);
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::getUKnot(PyObject *args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    double M = surf->UKnot(Index);

    return Py_BuildValue("d",M);
}

PyObject* BSplineSurfacePy::getVKnot(PyObject *args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    double M = surf->VKnot(Index);

    return Py_BuildValue("d",M);
}

PyObject* BSplineSurfacePy::setUKnots(PyObject *args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &obj))
        return 0;
    try {
        Py::List list(obj);
        TColStd_Array1OfReal k(1,list.size());
        int index=1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }

        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetUKnots(k);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setVKnots(PyObject *args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &obj))
        return 0;
    try {
        Py::List list(obj);
        TColStd_Array1OfReal k(1,list.size());
        int index=1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }

        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetVKnots(k);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getUKnots(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfReal w(1,surf->NbUKnots());
        surf->UKnots(w);
        Py::List knots;
        for (Standard_Integer i=w.Lower(); i<=w.Upper(); i++) {
            knots.append(Py::Float(w(i)));
        }
        return Py::new_reference_to(knots);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getVKnots(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfReal w(1,surf->NbVKnots());
        surf->VKnots(w);
        Py::List knots;
        for (Standard_Integer i=w.Lower(); i<=w.Upper(); i++) {
            knots.append(Py::Float(w(i)));
        }
        return Py::new_reference_to(knots);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setPole(PyObject *args)
{
    int uindex, vindex;
    double weight=-1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iiO!|d", &uindex,&vindex,&(Base::VectorPy::Type),&p,&weight))
        return 0;
    Base::Vector3d vec = static_cast<Base::VectorPy*>(p)->value();
    gp_Pnt pnt(vec.x, vec.y, vec.z);
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        if (weight < 0.0)
            surf->SetPole(uindex,vindex,pnt);
        else
            surf->SetPole(uindex,vindex,pnt,weight);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setPoleCol(PyObject *args)
{
    int vindex;
    PyObject* obj;
    PyObject* obj2=0;
    if (!PyArg_ParseTuple(args, "iO!|O!",&vindex,&PyList_Type,&obj,&PyList_Type,&obj2))
        return 0;
    try {
        Py::List list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        if (obj2 == 0) {
            surf->SetPoleCol(vindex, poles);
        }
        else {
            Py::List list(obj2);
            TColStd_Array1OfReal weights(1, list.size());
            int index=1;
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                weights(index++) = (double)Py::Float(*it);
            }
            surf->SetPoleCol(vindex, poles, weights);
        }

        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setPoleRow(PyObject *args)
{
    int uindex;
    PyObject* obj;
    PyObject* obj2=0;
    if (!PyArg_ParseTuple(args, "iO!|O!",&uindex,&PyList_Type,&obj,&PyList_Type,&obj2))
        return 0;
    try {
        Py::List list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        if (obj2 == 0) {
            surf->SetPoleRow(uindex, poles);
        }
        else {
            Py::List list(obj2);
            TColStd_Array1OfReal weights(1, list.size());
            int index=1;
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                weights(index++) = (double)Py::Float(*it);
            }
            surf->SetPoleRow(uindex, poles, weights);
        }

        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getPole(PyObject *args)
{
    int uindex,vindex;
    if (!PyArg_ParseTuple(args, "ii", &uindex,&vindex))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        gp_Pnt pnt = surf->Pole(uindex,vindex);
        Base::VectorPy* vec = new Base::VectorPy(Base::Vector3d(
            pnt.X(), pnt.Y(), pnt.Z()));
        return vec;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getPoles(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        TColgp_Array2OfPnt p(1,surf->NbUPoles(),1,surf->NbVPoles());
        surf->Poles(p);
        Py::List poles;
        for (Standard_Integer i=p.LowerRow(); i<=p.UpperRow(); i++) {
            Py::List row;
            for (Standard_Integer j=p.LowerCol(); j<=p.UpperCol(); j++) {
                const gp_Pnt& pole = p(i,j);
                row.append(Py::Object(new Base::VectorPy(
                    Base::Vector3d(pole.X(),pole.Y(),pole.Z()))));
            }
            poles.append(row);
        }
        return Py::new_reference_to(poles);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setWeight(PyObject *args)
{
    int uindex,vindex;
    double weight;
    if (!PyArg_ParseTuple(args, "iid",&uindex,&vindex,&weight))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetWeight(uindex,vindex,weight);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setWeightCol(PyObject *args)
{
    int vindex;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "iO!",&vindex,&PyList_Type,&obj))
        return 0;
    try {
        Py::List list(obj);
        TColStd_Array1OfReal weights(1, list.size());
        int index=1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            weights(index++) = (double)Py::Float(*it);
        }

        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetWeightCol(vindex, weights);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setWeightRow(PyObject *args)
{
    int uindex;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "iO!",&uindex,&PyList_Type,&obj))
        return 0;
    try {
        Py::List list(obj);
        TColStd_Array1OfReal weights(1, list.size());
        int index=1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            weights(index++) = (double)Py::Float(*it);
        }

        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetWeightRow(uindex, weights);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getWeight(PyObject *args)
{
    int uindex,vindex;
    if (!PyArg_ParseTuple(args, "ii",&uindex,&vindex))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        double w = surf->Weight(uindex,vindex);
        return Py_BuildValue("d", w);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getWeights(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array2OfReal w(1,surf->NbUPoles(),1,surf->NbVPoles());
        surf->Weights(w);
        Py::List weights;
        for (Standard_Integer i=w.LowerRow(); i<=w.UpperRow(); i++) {
            Py::List row;
            for (Standard_Integer j=w.LowerCol(); j<=w.UpperCol(); j++) {
                row.append(Py::Float(w(i,j)));
            }
            weights.append(row);
        }
        return Py::new_reference_to(weights);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getResolution(PyObject *args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        double utol, vtol;
        surf->Resolution(tol,utol,vtol);
        return Py_BuildValue("(dd)",utol,vtol);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::movePoint(PyObject *args)
{
    double U,V;
    int uindex1, uindex2;
    int vindex1, vindex2;
    PyObject* pnt;
    if (!PyArg_ParseTuple(args, "ddO!iiii", &U, &V, &(Base::VectorPy::Type),&pnt,
                                            &uindex1, &uindex2,&vindex1, &vindex2))
        return 0;
    try {
        Base::Vector3d p = static_cast<Base::VectorPy*>(pnt)->value();
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        int ufirst, ulast, vfirst, vlast;
        surf->MovePoint(U, V, gp_Pnt(p.x,p.y,p.z), uindex1, uindex2, vindex1, vindex2,
            ufirst, ulast, vfirst, vlast);
        return Py_BuildValue("(iiii)",ufirst, ulast, vfirst, vlast);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setUNotPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetUNotPeriodic();
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setVNotPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetVNotPeriodic();
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setUPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetUPeriodic();
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setVPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetVPeriodic();
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setUOrigin(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetUOrigin(index);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::setVOrigin(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        surf->SetVOrigin(index);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getUMultiplicity(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        int mult = surf->UMultiplicity(index);
        return Py_BuildValue("i", mult);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getVMultiplicity(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        int mult = surf->VMultiplicity(index);
        return Py_BuildValue("i", mult);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getUMultiplicities(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfInteger m(1,surf->NbUKnots());
        surf->UMultiplicities(m);
        Py::List mults;
        for (Standard_Integer i=m.Lower(); i<=m.Upper(); i++) {
            mults.append(Py::Int(m(i)));
        }
        return Py::new_reference_to(mults);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::getVMultiplicities(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfInteger m(1,surf->NbVKnots());
        surf->VMultiplicities(m);
        Py::List mults;
        for (Standard_Integer i=m.Lower(); i<=m.Upper(); i++) {
            mults.append(Py::Int(m(i)));
        }
        return Py::new_reference_to(mults);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::exchangeUV(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    surf->ExchangeUV();
    Py_Return;
}

PyObject* BSplineSurfacePy::uIso(PyObject * args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d", &u))
        return 0;

    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        Handle_Geom_Curve c = surf->UIso(u);
        return new BSplineCurvePy(new GeomBSplineCurve(Handle_Geom_BSplineCurve::DownCast(c)));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::vIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return 0;

    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());
        Handle_Geom_Curve c = surf->VIso(v);
        return new BSplineCurvePy(new GeomBSplineCurve(Handle_Geom_BSplineCurve::DownCast(c)));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineSurfacePy::reparametrize(PyObject * args)
{
    int u,v;
    double tol = 0.000001;
    if (!PyArg_ParseTuple(args, "ii|d", &u, &v, &tol))
        return 0;

    // u,v must be at least 2
    u = std::max<int>(u, 2);
    v = std::max<int>(v, 2);

    try {
        Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
            (getGeometryPtr()->handle());

        double maxU = surf->UKnot(surf->NbUKnots()); // 1.0 if normalized surface
        double maxV = surf->VKnot(surf->NbVKnots()); // 1.0 if normalized surface

        GeomBSplineSurface* geom = new GeomBSplineSurface();
        Handle_Geom_BSplineSurface spline = Handle_Geom_BSplineSurface::DownCast
            (geom->handle());
        for (int i=1; i<u-1; i++) {
            double U = i * 1.0 / (u-1.0);
            spline->InsertUKnot(U,i,tol,Standard_True);
        }

        for (int i=1; i<v-1; i++) {
            double V = i * 1.0 / (v-1.0);
            spline->InsertVKnot(V,i,tol,Standard_True);
        }

        for (int j=0; j<u; j++) {
            double U = j * maxU / (u-1.0);
            double newU = j * 1.0 / (u-1.0);
            for (int k=0; k<v; k++) {
                double V = k * maxV / (v-1.0);
                double newV = k * 1.0 / (v-1.0);
                // Get UV point and move new surface UV point
                gp_Pnt point = surf->Value(U,V);
                int ufirst, ulast, vfirst, vlast;
                spline->MovePoint(newU, newV, point, j+1, j+1, k+1, k+1, ufirst, ulast, vfirst, vlast);
            }
        }

        return new BSplineSurfacePy(geom);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

Py::Int BSplineSurfacePy::getUDegree(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    int deg = surf->UDegree();
    return Py::Int(deg);
}

Py::Int BSplineSurfacePy::getVDegree(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    int deg = surf->VDegree();
    return Py::Int(deg);
}

Py::Int BSplineSurfacePy::getMaxDegree(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->MaxDegree()); 
}

Py::Int BSplineSurfacePy::getNbUPoles(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->NbUPoles()); 
}

Py::Int BSplineSurfacePy::getNbVPoles(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->NbVPoles()); 
}

Py::Int BSplineSurfacePy::getNbUKnots(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->NbUKnots()); 
}

Py::Int BSplineSurfacePy::getNbVKnots(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->NbVKnots()); 
}

Py::Object BSplineSurfacePy::getFirstUKnotIndex(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    int index = surf->FirstUKnotIndex();
    return Py::Int(index);
}

Py::Object BSplineSurfacePy::getLastUKnotIndex(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    int index = surf->LastUKnotIndex();
    return Py::Int(index);
}

Py::Object BSplineSurfacePy::getFirstVKnotIndex(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    int index = surf->FirstVKnotIndex();
    return Py::Int(index);
}

Py::Object BSplineSurfacePy::getLastVKnotIndex(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    int index = surf->LastVKnotIndex();
    return Py::Int(index);
}

Py::List BSplineSurfacePy::getUKnotSequence(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    Standard_Integer m = 0;
    for (int i=1; i<= surf->NbUKnots(); i++)
        m += surf->UMultiplicity(i);
    TColStd_Array1OfReal k(1,m);
    surf->UKnotSequence(k);
    Py::List list;
    for (Standard_Integer i=k.Lower(); i<=k.Upper(); i++) {
        list.append(Py::Float(k(i)));
    }
    return list;
}

Py::List BSplineSurfacePy::getVKnotSequence(void) const
{
    Handle_Geom_BSplineSurface surf = Handle_Geom_BSplineSurface::DownCast
        (getGeometryPtr()->handle());
    Standard_Integer m = 0;
    for (int i=1; i<= surf->NbVKnots(); i++)
        m += surf->VMultiplicity(i);
    TColStd_Array1OfReal k(1,m);
    surf->VKnotSequence(k);
    Py::List list;
    for (Standard_Integer i=k.Lower(); i<=k.Upper(); i++) {
        list.append(Py::Float(k(i)));
    }
    return list;
}

PyObject *BSplineSurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int BSplineSurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
