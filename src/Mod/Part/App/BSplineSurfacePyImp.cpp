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
# include <Geom_BSplineSurface.hxx>
# include <GeomAbs_Shape.hxx>
# include <GeomAPI_PointsToBSplineSurface.hxx>
# include <Precision.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColgp_Array2OfPnt.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_Array2OfReal.hxx>
# include <TColStd_Array1OfInteger.hxx>
#endif
# include <GeomFill_NSections.hxx>

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "BSplineSurfacePy.h"
#include "BSplineSurfacePy.cpp"
#include "BSplineCurvePy.h"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BSplineSurfacePy::representation() const
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
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
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
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsURational();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineSurfacePy::isVRational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVRational();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineSurfacePy::isUPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineSurfacePy::isVPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineSurfacePy::isUClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineSurfacePy::isVClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BSplineSurfacePy::increaseDegree(PyObject *args)
{
    int udegree, vdegree;
    if (!PyArg_ParseTuple(args, "ii",&udegree,&vdegree))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    surf->IncreaseDegree(udegree,vdegree);
    Py_Return;
}

PyObject* BSplineSurfacePy::increaseUMultiplicity(PyObject *args)
{
    int mult=-1;
    int start, end;
    if (!PyArg_ParseTuple(args, "ii|i", &start, &end, &mult))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
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
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
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
        return nullptr;

    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->IncrementUMultiplicity(start, end, mult);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::incrementVMultiplicity(PyObject *args)
{
    int start, end, mult;
    if (!PyArg_ParseTuple(args, "iii", &start, &end, &mult))
        return nullptr;

    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->IncrementVMultiplicity(start, end, mult);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::insertUKnot(PyObject *args)
{
    double U, tol = 0.0;
    int M=1;
    PyObject* add = Py_True;
    if (!PyArg_ParseTuple(args, "did|O!", &U, &M, &tol, &PyBool_Type, &add))
        return nullptr;

    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->InsertUKnot(U, M, tol, Base::asBoolean(add));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::insertUKnots(PyObject *args)
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
            Py::Long val(*it);
            m(index++) = (int)val;
        }

        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->InsertUKnots(k, m, tol, Base::asBoolean(add));
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::insertVKnot(PyObject *args)
{
    double V, tol = 0.0;
    int M=1;
    PyObject* add = Py_True;
    if (!PyArg_ParseTuple(args, "did|O!", &V, &M, &tol, &PyBool_Type, &add))
        return nullptr;

    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->InsertVKnot(V, M, tol, Base::asBoolean(add));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::insertVKnots(PyObject *args)
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
            Py::Long val(*it);
            m(index++) = (int)val;
        }

        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->InsertVKnots(k, m, tol, Base::asBoolean(add));
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::removeUKnot(PyObject *args)
{
    double tol;
    int Index,M;
    if (!PyArg_ParseTuple(args, "iid", &Index, &M, &tol))
        return nullptr;

    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        Standard_Boolean ok = surf->RemoveUKnot(Index,M,tol);
        return PyBool_FromLong(ok ? 1 : 0);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::removeVKnot(PyObject *args)
{
    double tol;
    int Index,M;
    if (!PyArg_ParseTuple(args, "iid", &Index, &M, &tol))
        return nullptr;

    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        Standard_Boolean ok = surf->RemoveVKnot(Index,M,tol);
        return PyBool_FromLong(ok ? 1 : 0);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::segment(PyObject *args)
{
    double u1,u2,v1,v2;
    if (!PyArg_ParseTuple(args, "dddd", &u1,&u2,&v1,&v2))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->Segment(u1,u2,v1,v2);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setUKnot(PyObject *args)
{
    int Index, M=-1;
    double K;
    if (!PyArg_ParseTuple(args, "id|i", &Index, &K, &M))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
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
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    if (M == -1) {
        surf->SetVKnot(Index, K);
    }
    else {
        surf->SetVKnot(Index, K, M);
    }

    Py_Return;
}

PyObject* BSplineSurfacePy::getUKnot(PyObject *args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    double M = surf->UKnot(Index);

    return Py_BuildValue("d",M);
}

PyObject* BSplineSurfacePy::getVKnot(PyObject *args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    double M = surf->VKnot(Index);

    return Py_BuildValue("d",M);
}

PyObject* BSplineSurfacePy::setUKnots(PyObject *args)
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

        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetUKnots(k);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setVKnots(PyObject *args)
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

        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetVKnots(k);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getUKnots(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfReal w(1,surf->NbUKnots());
        surf->UKnots(w);
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

PyObject* BSplineSurfacePy::getVKnots(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfReal w(1,surf->NbVKnots());
        surf->VKnots(w);
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

PyObject* BSplineSurfacePy::setPole(PyObject *args)
{
    int uindex, vindex;
    double weight=-1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iiO!|d", &uindex,&vindex,&(Base::VectorPy::Type),&p,&weight))
        return nullptr;
    Base::Vector3d vec = static_cast<Base::VectorPy*>(p)->value();
    gp_Pnt pnt(vec.x, vec.y, vec.z);
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        if (weight < 0.0)
            surf->SetPole(uindex,vindex,pnt);
        else
            surf->SetPole(uindex,vindex,pnt,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setPoleCol(PyObject *args)
{
    int vindex;
    PyObject* obj;
    PyObject* obj2=nullptr;
    if (!PyArg_ParseTuple(args, "iO|O",&vindex,&obj,&obj2))
        return nullptr;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        if (!obj2) {
            surf->SetPoleCol(vindex, poles);
        }
        else {
            Py::Sequence list(obj2);
            TColStd_Array1OfReal weights(1, list.size());
            int index=1;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                weights(index++) = (double)Py::Float(*it);
            }
            surf->SetPoleCol(vindex, poles, weights);
        }

        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setPoleRow(PyObject *args)
{
    int uindex;
    PyObject* obj;
    PyObject* obj2=nullptr;
    if (!PyArg_ParseTuple(args, "iO|O",&uindex,&obj,&obj2))
        return nullptr;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        if (!obj2) {
            surf->SetPoleRow(uindex, poles);
        }
        else {
            Py::Sequence list(obj2);
            TColStd_Array1OfReal weights(1, list.size());
            int index=1;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                weights(index++) = (double)Py::Float(*it);
            }
            surf->SetPoleRow(uindex, poles, weights);
        }

        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getPole(PyObject *args)
{
    int uindex,vindex;
    if (!PyArg_ParseTuple(args, "ii", &uindex,&vindex))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        Standard_OutOfRange_Raise_if
            (uindex < 1 || uindex > surf->NbUPoles() ||
             vindex < 1 || vindex > surf->NbVPoles(), "Pole index out of range");
        gp_Pnt pnt = surf->Pole(uindex,vindex);
        Base::VectorPy* vec = new Base::VectorPy(Base::Vector3d(
            pnt.X(), pnt.Y(), pnt.Z()));
        return vec;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getPoles(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        TColgp_Array2OfPnt p(1,surf->NbUPoles(),1,surf->NbVPoles());
        surf->Poles(p);
        Py::List poles;
        for (Standard_Integer i=p.LowerRow(); i<=p.UpperRow(); i++) {
            Py::List row;
            for (Standard_Integer j=p.LowerCol(); j<=p.UpperCol(); j++) {
                const gp_Pnt& pole = p(i,j);
                row.append(Py::asObject(new Base::VectorPy(
                    Base::Vector3d(pole.X(),pole.Y(),pole.Z()))));
            }
            poles.append(row);
        }
        return Py::new_reference_to(poles);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setWeight(PyObject *args)
{
    int uindex,vindex;
    double weight;
    if (!PyArg_ParseTuple(args, "iid",&uindex,&vindex,&weight))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetWeight(uindex,vindex,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setWeightCol(PyObject *args)
{
    int vindex;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "iO",&vindex,&obj))
        return nullptr;
    try {
        Py::Sequence list(obj);
        TColStd_Array1OfReal weights(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            weights(index++) = (double)Py::Float(*it);
        }

        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetWeightCol(vindex, weights);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setWeightRow(PyObject *args)
{
    int uindex;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "iO",&uindex,&obj))
        return nullptr;
    try {
        Py::Sequence list(obj);
        TColStd_Array1OfReal weights(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            weights(index++) = (double)Py::Float(*it);
        }

        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetWeightRow(uindex, weights);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getWeight(PyObject *args)
{
    int uindex,vindex;
    if (!PyArg_ParseTuple(args, "ii",&uindex,&vindex))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        Standard_OutOfRange_Raise_if
            (uindex < 1 || uindex > surf->NbUPoles() ||
             vindex < 1 || vindex > surf->NbVPoles(), "Weight index out of range");
        double w = surf->Weight(uindex,vindex);
        return Py_BuildValue("d", w);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getWeights(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
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
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getPolesAndWeights(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        TColgp_Array2OfPnt p(1,surf->NbUPoles(),1,surf->NbVPoles());
        surf->Poles(p);
        TColStd_Array2OfReal w(1,surf->NbUPoles(),1,surf->NbVPoles());
        surf->Weights(w);

        Py::List poles;
        for (Standard_Integer i=p.LowerRow(); i<=p.UpperRow(); i++) {
            Py::List row;
            for (Standard_Integer j=p.LowerCol(); j<=p.UpperCol(); j++) {
                const gp_Pnt& pole = p(i,j);
                double weight = w(i,j);
                Py::Tuple t(4);
                t.setItem(0, Py::Float(pole.X()));
                t.setItem(1, Py::Float(pole.Y()));
                t.setItem(2, Py::Float(pole.Z()));
                t.setItem(3, Py::Float(weight));
                row.append(t);
            }
            poles.append(row);
        }
        return Py::new_reference_to(poles);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getResolution(PyObject *args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        double utol, vtol;
        surf->Resolution(tol,utol,vtol);
        return Py_BuildValue("(dd)",utol,vtol);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
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
        return nullptr;
    try {
        Base::Vector3d p = static_cast<Base::VectorPy*>(pnt)->value();
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        int ufirst, ulast, vfirst, vlast;
        surf->MovePoint(U, V, gp_Pnt(p.x,p.y,p.z), uindex1, uindex2, vindex1, vindex2,
            ufirst, ulast, vfirst, vlast);
        return Py_BuildValue("(iiii)",ufirst, ulast, vfirst, vlast);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setUNotPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetUNotPeriodic();
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setVNotPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetVNotPeriodic();
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setUPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetUPeriodic();
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setVPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetVPeriodic();
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setUOrigin(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetUOrigin(index);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::setVOrigin(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetVOrigin(index);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getUMultiplicity(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        int mult = surf->UMultiplicity(index);
        return Py_BuildValue("i", mult);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getVMultiplicity(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        int mult = surf->VMultiplicity(index);
        return Py_BuildValue("i", mult);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::getUMultiplicities(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfInteger m(1,surf->NbUKnots());
        surf->UMultiplicities(m);
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

PyObject* BSplineSurfacePy::getVMultiplicities(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfInteger m(1,surf->NbVKnots());
        surf->VMultiplicities(m);
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

PyObject* BSplineSurfacePy::exchangeUV(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    surf->ExchangeUV();
    Py_Return;
}

PyObject* BSplineSurfacePy::reparametrize(PyObject * args)
{
    int u,v;
    double tol = 0.000001;
    if (!PyArg_ParseTuple(args, "ii|d", &u, &v, &tol))
        return nullptr;

    // u,v must be at least 2
    u = std::max<int>(u, 2);
    v = std::max<int>(v, 2);

    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
            (getGeometryPtr()->handle());

        double maxU = surf->UKnot(surf->NbUKnots()); // 1.0 if normalized surface
        double maxV = surf->VKnot(surf->NbVKnots()); // 1.0 if normalized surface

        GeomBSplineSurface* geom = new GeomBSplineSurface();
        Handle(Geom_BSplineSurface) spline = Handle(Geom_BSplineSurface)::DownCast
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
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::approximate(PyObject *args, PyObject *kwds)
{
    PyObject* obj;
    Standard_Integer degMin=3;
    Standard_Integer degMax=8;
    Standard_Integer continuity=2;
    Standard_Real tol3d = Precision::Approximation();
    char* parType = "None";
    Standard_Real weight1 = 1.0;
    Standard_Real weight2 = 1.0;
    Standard_Real weight3 = 1.0;
    Standard_Real X0=0;
    Standard_Real dX=0;
    Standard_Real Y0=0;
    Standard_Real dY=0;

    static const std::array<const char *, 14> kwds_interp{"Points", "DegMin", "DegMax", "Continuity", "Tolerance", "X0",
                                                          "dX", "Y0", "dY", "ParamType", "LengthWeight",
                                                          "CurvatureWeight", "TorsionWeight", nullptr};

    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O|iiidddddsddd", kwds_interp, &obj, &degMin, &degMax,
                                             &continuity, &tol3d, &X0, &dX, &Y0, &dY, &parType, &weight1, &weight2,
                                             &weight3)) {
        return nullptr;
    }
    try {
        Py::Sequence list(obj);
        Standard_Integer lu = list.size();
        Py::Sequence col(list.getItem(0));
        Standard_Integer lv = col.size();
        TColgp_Array2OfPnt interpolationPoints(1, lu, 1, lv);
        TColStd_Array2OfReal zPoints(1, lu, 1, lv);
        //Base::Console().Message("lu=%d, lv=%d\n", lu, lv);

        Standard_Integer index1 = 0;
        Standard_Integer index2 = 0;
        for (Py::Sequence::iterator it1 = list.begin(); it1 != list.end(); ++it1) {
            index1++;
            index2=0;
            Py::Sequence row(*it1);
            for (Py::Sequence::iterator it2 = row.begin(); it2 != row.end(); ++it2) {
                index2++;
                if ((dX == 0) || (dY == 0)){
                    Py::Vector v(*it2);
                    Base::Vector3d pnt = v.toVector();
                    gp_Pnt newPoint(pnt.x,pnt.y,pnt.z);
                    interpolationPoints.SetValue(index1, index2, newPoint);
                }
                else {
                    Standard_Real val = PyFloat_AsDouble((*it2).ptr());
                    zPoints.SetValue(index1, index2, val);
                }
            }
        }

        if (continuity<0 || continuity>2) {
            Standard_Failure::Raise("continuity must be between 0 and 2");
        }

        if (interpolationPoints.RowLength() < 2 || interpolationPoints.ColLength() < 2) {
            Standard_Failure::Raise("not enough points given");
        }

        GeomAbs_Shape c = GeomAbs_C2;
        switch(continuity){
        case 0:
            c = GeomAbs_C0;
            break;
        case 1:
            c = GeomAbs_C1;
            break;
        case 2:
            c = GeomAbs_C2;
            break;
        }

        Approx_ParametrizationType pt;
        std::string pstr = parType;
        Standard_Boolean useParam = Standard_True;
        if (pstr == "Uniform" )
            pt = Approx_IsoParametric;
        else if (pstr == "Centripetal" )
            pt = Approx_Centripetal;
        else if (pstr == "ChordLength" )
            pt = Approx_ChordLength;
        else
            useParam = Standard_False;

        GeomAPI_PointsToBSplineSurface surInterpolation;
        if (!(dX == 0) && !(dY == 0)) {
            // dX and dY are not null : we use the zPoints method
            surInterpolation.Init(zPoints, X0, dX, Y0, dY, degMin, degMax, c, tol3d);
        }
        else if (useParam) {
            // a parametrization type has been supplied
            surInterpolation.Init(interpolationPoints, pt, degMin, degMax, c, tol3d);
        }
        else if (!(weight1 == 0) || !(weight2 == 0) || !(weight3 == 0)) {
            // one of the weights is not null, we use the smoothing algorithm
            surInterpolation.Init(interpolationPoints, weight1, weight2, weight3, degMax, c, tol3d);
        }
        else {
            // fallback to strandard method
            surInterpolation.Init(interpolationPoints, degMin, degMax, c, tol3d);
        }
        Handle(Geom_BSplineSurface) sur(surInterpolation.Surface());
        this->getGeomBSplineSurfacePtr()->setHandle(sur);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        std::string err = e.GetMessageString();
        if (err.empty()) err = e.DynamicType()->Name();
        PyErr_SetString(PartExceptionOCCError, err.c_str());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::interpolate(PyObject *args)
{
    PyObject* obj;
    Standard_Real X0=0;
    Standard_Real dX=0;
    Standard_Real Y0=0;
    Standard_Real dY=0;

    int len = PyTuple_GET_SIZE(args);

    if (!PyArg_ParseTuple(args, "O|dddd", &obj, &X0, &dX, &Y0, &dY))
        return nullptr;
    try {
        Py::Sequence list(obj);
        Standard_Integer lu = list.size();
        Py::Sequence col(list.getItem(0));
        Standard_Integer lv = col.size();
        TColgp_Array2OfPnt interpolationPoints(1, lu, 1, lv);
        TColStd_Array2OfReal zPoints(1, lu, 1, lv);

        Standard_Integer index1 = 0;
        Standard_Integer index2 = 0;
        for (Py::Sequence::iterator it1 = list.begin(); it1 != list.end(); ++it1) {
            index1++;
            index2=0;
            Py::Sequence row(*it1);
            for (Py::Sequence::iterator it2 = row.begin(); it2 != row.end(); ++it2) {
                index2++;
                if(len == 1){
                    Py::Vector v(*it2);
                    Base::Vector3d pnt = v.toVector();
                    gp_Pnt newPoint(pnt.x,pnt.y,pnt.z);
                    interpolationPoints.SetValue(index1, index2, newPoint);
                }
                else {
                    Standard_Real val = PyFloat_AsDouble((*it2).ptr());
                    zPoints.SetValue(index1, index2, val);
                }
            }
        }

        if (interpolationPoints.RowLength() < 2 || interpolationPoints.ColLength() < 2) {
            Standard_Failure::Raise("not enough points given");
        }

        GeomAPI_PointsToBSplineSurface surInterpolation;
        if(len == 1){
            surInterpolation.Interpolate (interpolationPoints);
        }
        else {
            surInterpolation.Interpolate(zPoints, X0, dX, Y0, dY);
        }
        Handle(Geom_BSplineSurface) sur(surInterpolation.Surface());
        this->getGeomBSplineSurfacePtr()->setHandle(sur);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        std::string err = e.GetMessageString();
        if (err.empty()) err = e.DynamicType()->Name();
        PyErr_SetString(PartExceptionOCCError, err.c_str());
        return nullptr;
    }
}

PyObject* BSplineSurfacePy::buildFromPolesMultsKnots(PyObject *args, PyObject *keywds)
{
    static const std::array<const char *, 11> kwlist{"poles", "umults", "vmults", "uknots", "vknots", "uperiodic",
                                                     "vperiodic", "udegree", "vdegree", "weights", nullptr};
    PyObject* uperiodic = Py_False; // NOLINT
    PyObject* vperiodic = Py_False; // NOLINT
    PyObject* poles = Py_None;
    PyObject* umults = Py_None;
    PyObject* vmults = Py_None;
    PyObject* uknots = Py_None;
    PyObject* vknots = Py_None;
    PyObject* weights = Py_None;
    int udegree = 3;
    int vdegree = 3;
    int number_of_uknots = 0;
    int number_of_vknots = 0;
    int sum_of_umults = 0;
    int sum_of_vmults = 0;

    if (!Base::Wrapped_ParseTupleAndKeywords(args, keywds, "OOO|OOO!O!iiO", kwlist,
                                             &poles, &umults, &vmults, //required
                                             &uknots, &vknots, //optional
                                             &PyBool_Type, &uperiodic, &PyBool_Type, &vperiodic, //optional
                                             &udegree, &vdegree, &weights)) {
        return nullptr;
    }
    try {
        Py::Sequence list(poles);
        Standard_Integer lu = list.size();
        Py::Sequence col(list.getItem(0));
        Standard_Integer lv = col.size();
        TColgp_Array2OfPnt occpoles(1, lu, 1, lv);
        TColStd_Array2OfReal occweights(1, lu, 1, lv);
        Standard_Boolean genweights = (weights==Py_None) ? Standard_True : Standard_False; //cache
        Standard_Integer index1 = 0;
        Standard_Integer index2 = 0;
        for (Py::Sequence::iterator it1 = list.begin(); it1 != list.end(); ++it1) {
            index1++;
            index2=0;
            Py::Sequence row(*it1);
            for (Py::Sequence::iterator it2 = row.begin(); it2 != row.end(); ++it2) {
                index2++;
                Py::Vector v(*it2);
                Base::Vector3d pnt = v.toVector();
                gp_Pnt newPoint(pnt.x,pnt.y,pnt.z);
                occpoles.SetValue(index1, index2, newPoint);
                if (genweights) occweights.SetValue(index1, index2, 1.0); //set weights if they are not given
            }
        }
        if (occpoles.RowLength() < 2 || occpoles.ColLength() < 2) {
            Standard_Failure::Raise("not enough points given");
        }
        if (!genweights) {//copy the weights
            Py::Sequence list(weights);
            Standard_Integer lwu = list.size();
            Py::Sequence col(list.getItem(0));
            Standard_Integer lwv = col.size();
            if (lwu != lu || lwv != lv) { Standard_Failure::Raise("weights and poles mismatch");}
            Standard_Integer index1 = 0;
            Standard_Integer index2 = 0;
            for (Py::Sequence::iterator it1 = list.begin(); it1 != list.end(); ++it1) {
                index1++;
                index2=0;
                Py::Sequence row(*it1);
                for (Py::Sequence::iterator it2 = row.begin(); it2 != row.end(); ++it2) {
                    index2++;
                    Py::Float f(*it2);
                    occweights.SetValue(index1, index2, f);
                }
            }
        }
        number_of_uknots = PyObject_Length(umults);
        number_of_vknots = PyObject_Length(vmults);
        if (((uknots != Py_None) && PyObject_Length(uknots) != number_of_uknots) ||
                ((vknots != Py_None) && PyObject_Length(vknots) != number_of_vknots)){
            Standard_Failure::Raise("number of knots and mults mismatch");
            return nullptr;
        }
        //copy mults
        TColStd_Array1OfInteger occumults(1,number_of_uknots);
        TColStd_Array1OfInteger occvmults(1,number_of_vknots);
        TColStd_Array1OfReal occuknots(1,number_of_uknots);
        TColStd_Array1OfReal occvknots(1,number_of_vknots);
        Py::Sequence umultssq(umults);
        Standard_Integer index = 1;
        for (Py::Sequence::iterator it = umultssq.begin(); it != umultssq.end() && index <= occumults.Length(); ++it) {
            Py::Long mult(*it);
            if (index < occumults.Length() || !Base::asBoolean(uperiodic)) {
                sum_of_umults += static_cast<int>(mult); //sum up the mults to compare them against the number of poles later
            }
            occumults(index++) = static_cast<int>(mult);
        }
        Py::Sequence vmultssq(vmults);
        index = 1;
        for (Py::Sequence::iterator it = vmultssq.begin(); it != vmultssq.end() && index <= occvmults.Length(); ++it) {
            Py::Long mult(*it);
            if (index < occvmults.Length() || !Base::asBoolean(vperiodic)) {
                sum_of_vmults += static_cast<int>(mult); //sum up the mults to compare them against the number of poles later
            }
            occvmults(index++) = static_cast<int>(mult);
        }
        //copy or generate knots
        if (uknots != Py_None) { //uknots are given
            Py::Sequence uknotssq(uknots);
            index = 1;
            for (Py::Sequence::iterator it = uknotssq.begin(); it != uknotssq.end() && index <= occuknots.Length(); ++it) {
                Py::Float knot(*it);
                occuknots(index++) = knot;
            }
        }
        else { // knotes are uniformly spaced 0..1 if not given
            for (int i=1; i<=occuknots.Length(); i++){
                occuknots.SetValue(i,(double)(i-1)/(occuknots.Length()-1));
            }
        }
        if (vknots != Py_None) { //vknots are given
            Py::Sequence vknotssq(vknots);
            index = 1;
            for (Py::Sequence::iterator it = vknotssq.begin(); it != vknotssq.end() && index <= occvknots.Length(); ++it) {
                Py::Float knot(*it);
                occvknots(index++) = knot;
            }
        }
        else { // knotes are uniformly spaced 0..1 if not given
            for (int i=1; i<=occvknots.Length(); i++){
                occvknots.SetValue(i,(double)(i-1)/(occvknots.Length()-1));
            }
        }
        if ((Base::asBoolean(uperiodic) && sum_of_umults != lu) ||
            (!Base::asBoolean(uperiodic) && sum_of_umults - udegree -1 != lu) ||
            (Base::asBoolean(vperiodic) && sum_of_vmults != lv) ||
            (!Base::asBoolean(vperiodic) && sum_of_vmults - vdegree -1 != lv)) {
            Standard_Failure::Raise("number of poles and sum of mults mismatch");
        }
        // check multiplicity of inner knots
        for (Standard_Integer i=2; i < occumults.Length(); i++) {
            if (occumults(i) > udegree) {
                Standard_Failure::Raise("multiplicity of inner knot higher than degree");
            }
        }
        for (Standard_Integer i=2; i < occvmults.Length(); i++) {
            if (occvmults(i) > vdegree) {
                Standard_Failure::Raise("multiplicity of inner knot higher than degree");
            }
        }

        Handle(Geom_BSplineSurface) spline = new Geom_BSplineSurface(occpoles,occweights,
            occuknots,occvknots,occumults,occvmults,udegree,vdegree,
            Base::asBoolean(uperiodic),
            Base::asBoolean(vperiodic));
        if (!spline.IsNull()) {
            this->getGeomBSplineSurfacePtr()->setHandle(spline);
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

/*!
 * \code
import math
c = Part.Circle()
c.Radius=50
c = c.trim(0, math.pi)

e1 = Part.Ellipse()
e1.Center = (0, 0, 75)
e1.MajorRadius = 30
e1.MinorRadius = 5
e1 = e1.trim(0, math.pi)

e2 = Part.Ellipse()
e2.Center = (0, 0, 100)
e2.MajorRadius = 20
e2.MinorRadius = 5
e2 = e2.trim(0, math.pi)

bs = Part.BSplineSurface()
bs.buildFromNSections([c, e1, e2])
 * \endcode
 */
PyObject* BSplineSurfacePy::buildFromNSections(PyObject *args)
{
    PyObject* list;
    PyObject* refSurf = Py_False;
    if (!PyArg_ParseTuple(args, "O|O!", &list, &PyBool_Type, &refSurf))
        return nullptr;

    try {
        TColGeom_SequenceOfCurve curveSeq;
        Py::Sequence curves(list);
        for (Py::Sequence::iterator it = curves.begin(); it != curves.end(); ++it) {
            Py::Object obj(*it);
            if (PyObject_TypeCheck(obj.ptr(), &GeometryCurvePy::Type)) {
                GeomCurve* geom = static_cast<GeometryCurvePy*>(obj.ptr())->getGeomCurvePtr();
                curveSeq.Append(Handle(Geom_Curve)::DownCast(geom->handle()));
            }
        }

        GeomFill_NSections fillOp(curveSeq);
        if (Base::asBoolean(refSurf)) {
            Handle(Geom_BSplineSurface) ref = Handle(Geom_BSplineSurface)::DownCast
                (getGeometryPtr()->handle());
            fillOp.SetSurface(ref);
        }

        fillOp.ComputeSurface();

        Handle(Geom_BSplineSurface) aSurf = fillOp.BSplineSurface();
        this->getGeomBSplineSurfacePtr()->setHandle(aSurf);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        Standard_CString msg = e.GetMessageString();
        PyErr_SetString(PartExceptionOCCError, msg  ? msg : "");
        return nullptr;
    }
}

Py::Long BSplineSurfacePy::getUDegree() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    int deg = surf->UDegree();
    return Py::Long(deg);
}

Py::Long BSplineSurfacePy::getVDegree() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    int deg = surf->VDegree();
    return Py::Long(deg);
}

Py::Long BSplineSurfacePy::getMaxDegree() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->MaxDegree());
}

Py::Long BSplineSurfacePy::getNbUPoles() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->NbUPoles());
}

Py::Long BSplineSurfacePy::getNbVPoles() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->NbVPoles());
}

Py::Long BSplineSurfacePy::getNbUKnots() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->NbUKnots());
}

Py::Long BSplineSurfacePy::getNbVKnots() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->NbVKnots());
}

Py::Object BSplineSurfacePy::getFirstUKnotIndex() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    int index = surf->FirstUKnotIndex();
    return Py::Long(index);
}

Py::Object BSplineSurfacePy::getLastUKnotIndex() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    int index = surf->LastUKnotIndex();
    return Py::Long(index);
}

Py::Object BSplineSurfacePy::getFirstVKnotIndex() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    int index = surf->FirstVKnotIndex();
    return Py::Long(index);
}

Py::Object BSplineSurfacePy::getLastVKnotIndex() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    int index = surf->LastVKnotIndex();
    return Py::Long(index);
}

Py::List BSplineSurfacePy::getUKnotSequence() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Integer m = 0;
    if (surf->IsUPeriodic()) {
        // knots=poles+2*degree-mult(1)+2
        m = surf->NbUPoles() + 2*surf->UDegree() - surf->UMultiplicity(1) + 2;
    }
    else {
        for (int i=1; i<= surf->NbUKnots(); i++)
            m += surf->UMultiplicity(i);
    }
    TColStd_Array1OfReal k(1,m);
    surf->UKnotSequence(k);
    Py::List list;
    for (Standard_Integer i=k.Lower(); i<=k.Upper(); i++) {
        list.append(Py::Float(k(i)));
    }
    return list;
}

Py::List BSplineSurfacePy::getVKnotSequence() const
{
    Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Integer m = 0;
    if (surf->IsVPeriodic()) {
        // knots=poles+2*degree-mult(1)+2
        m = surf->NbVPoles() + 2*surf->VDegree() - surf->VMultiplicity(1) + 2;
    }
    else {
        for (int i=1; i<= surf->NbVKnots(); i++)
            m += surf->VMultiplicity(i);
    }
    TColStd_Array1OfReal k(1,m);
    surf->VKnotSequence(k);
    Py::List list;
    for (Standard_Integer i=k.Lower(); i<=k.Upper(); i++) {
        list.append(Py::Float(k(i)));
    }
    return list;
}

PyObject* BSplineSurfacePy::scaleKnotsToBounds(PyObject *args)
{
    double u0=0.0;
    double u1=1.0;
    double v0=0.0;
    double v1=1.0;

    if (!PyArg_ParseTuple(args, "|dddd", &u0, &u1, &v0, &v1))
        return nullptr;
    try {
        if (u0 >= u1 || v0 >= v1) {
            Standard_Failure::Raise("Bad parameter range");
            return nullptr;;
        }
        GeomBSplineSurface* surf = getGeomBSplineSurfacePtr();
        surf->scaleKnotsToBounds(u0, u1, v0, v1);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        std::string err = e.GetMessageString();
        if (err.empty()) err = e.DynamicType()->Name();
        PyErr_SetString(PartExceptionOCCError, err.c_str());
        return nullptr;
    }
}

PyObject *BSplineSurfacePy::getCustomAttributes(const char* attr) const
{
    // for backward compatibility
    if (strcmp(attr, "setBounds") == 0) {
        return PyObject_GetAttrString(const_cast<BSplineSurfacePy*>(this), "scaleKnotsToBounds");
    }
    return nullptr;
}

int BSplineSurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
