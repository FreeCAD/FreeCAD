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
# include <Geom_BezierCurve.hxx>
# include <Geom_BezierSurface.hxx>
# include <Handle_Geom_BezierCurve.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_Array2OfReal.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColgp_Array2OfPnt.hxx>
#endif

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "Geometry.h"
#include "BezierCurvePy.h"
#include "BezierSurfacePy.h"
#include "BezierSurfacePy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BezierSurfacePy::representation(void) const
{
    return "<BezierSurface object>";
}

PyObject *BezierSurfacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BezierSurfacePy and the Twin object 
    return new BezierSurfacePy(new GeomBezierSurface);
}

// constructor method
int BezierSurfacePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* BezierSurfacePy::bounds(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::isURational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::isVRational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::isUPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::isVPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::isUClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::isVClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::increase(PyObject *args)
{
    int udegree,vdegree;
    if (!PyArg_ParseTuple(args, "ii", &udegree, &vdegree))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        surf->Increase(udegree, vdegree);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::insertPoleColAfter(PyObject *args)
{
    int vindex;
    PyObject* obj;
    PyObject* obj2=0;
    if (!PyArg_ParseTuple(args, "iO|O",&vindex,&obj,&obj2))
        return 0;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        if (obj2 == 0) {
            surf->InsertPoleColAfter(vindex, poles);
        }
        else {
            Py::Sequence list(obj2);
            TColStd_Array1OfReal weights(1, list.size());
            int index=1;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                weights(index++) = (double)Py::Float(*it);
            }
            surf->InsertPoleColAfter(vindex, poles, weights);
        }

        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::insertPoleRowAfter(PyObject *args)
{
    int uindex;
    PyObject* obj;
    PyObject* obj2=0;
    if (!PyArg_ParseTuple(args, "iO|O",&uindex,&obj,&obj2))
        return 0;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        if (obj2 == 0) {
            surf->InsertPoleRowAfter(uindex, poles);
        }
        else {
            Py::Sequence list(obj2);
            TColStd_Array1OfReal weights(1, list.size());
            int index=1;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                weights(index++) = (double)Py::Float(*it);
            }
            surf->InsertPoleRowAfter(uindex, poles, weights);
        }

        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::insertPoleColBefore(PyObject *args)
{
    int vindex;
    PyObject* obj;
    PyObject* obj2=0;
    if (!PyArg_ParseTuple(args, "iO|O",&vindex,&obj,&obj2))
        return 0;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        if (obj2 == 0) {
            surf->InsertPoleColBefore(vindex, poles);
        }
        else {
            Py::Sequence list(obj2);
            TColStd_Array1OfReal weights(1, list.size());
            int index=1;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                weights(index++) = (double)Py::Float(*it);
            }
            surf->InsertPoleColBefore(vindex, poles, weights);
        }

        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::insertPoleRowBefore(PyObject *args)
{
    int uindex;
    PyObject* obj;
    PyObject* obj2=0;
    if (!PyArg_ParseTuple(args, "iO|O",&uindex,&obj,&obj2))
        return 0;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        if (obj2 == 0) {
            surf->InsertPoleRowBefore(uindex, poles);
        }
        else {
            Py::Sequence list(obj2);
            TColStd_Array1OfReal weights(1, list.size());
            int index=1;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                weights(index++) = (double)Py::Float(*it);
            }
            surf->InsertPoleRowBefore(uindex, poles, weights);
        }

        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::removePoleCol(PyObject *args)
{
    int vindex;
    if (!PyArg_ParseTuple(args, "i",&vindex))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        surf->RemovePoleCol(vindex);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::removePoleRow(PyObject *args)
{
    int uindex;
    if (!PyArg_ParseTuple(args, "i",&uindex))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        surf->RemovePoleRow(uindex);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::segment(PyObject *args)
{
    Standard_Real u1,u2,v1,v2;
    if (!PyArg_ParseTuple(args, "dddd",&u1,&u2,&v1,&v2))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::setPole(PyObject *args)
{
    int uindex,vindex;
    PyObject* obj;
    double weight=0.0;
    if (!PyArg_ParseTuple(args, "iiO!|d",&uindex,&vindex,&(Base::VectorPy::Type),&obj,&weight))
        return 0;
    try {
        Base::Vector3d pole = static_cast<Base::VectorPy*>(obj)->value();
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        if (weight <= gp::Resolution())
            surf->SetPole(uindex,vindex,gp_Pnt(pole.x,pole.y,pole.z));
        else
            surf->SetPole(uindex,vindex,gp_Pnt(pole.x,pole.y,pole.z),weight);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::setPoleCol(PyObject *args)
{
    int vindex;
    PyObject* obj;
    PyObject* obj2=0;
    if (!PyArg_ParseTuple(args, "iO|O",&vindex,&obj,&obj2))
        return 0;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        if (obj2 == 0) {
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
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::setPoleRow(PyObject *args)
{
    int uindex;
    PyObject* obj;
    PyObject* obj2=0;
    if (!PyArg_ParseTuple(args, "iO|O",&uindex,&obj,&obj2))
        return 0;
    try {
        Py::Sequence list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector p(*it);
            Base::Vector3d v = p.toVector();
            poles(index++) = gp_Pnt(v.x,v.y,v.z);
        }

        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        if (obj2 == 0) {
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
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::getPole(PyObject *args)
{
    int uindex,vindex;
    if (!PyArg_ParseTuple(args, "ii",&uindex,&vindex))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        Standard_OutOfRange_Raise_if
            (uindex < 1 || uindex > surf->NbUPoles() ||
             vindex < 1 || vindex > surf->NbVPoles(), "Pole index out of range");
        gp_Pnt p = surf->Pole(uindex,vindex);
        return new Base::VectorPy(Base::Vector3d(p.X(),p.Y(),p.Z()));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::getPoles(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::setWeight(PyObject *args)
{
    int uindex,vindex;
    double weight;
    if (!PyArg_ParseTuple(args, "iid",&uindex,&vindex,&weight))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::setWeightCol(PyObject *args)
{
    int vindex;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "iO",&vindex,&obj))
        return 0;
    try {
        Py::Sequence list(obj);
        TColStd_Array1OfReal weights(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            weights(index++) = (double)Py::Float(*it);
        }

        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::setWeightRow(PyObject *args)
{
    int uindex;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "iO",&uindex,&obj))
        return 0;
    try {
        Py::Sequence list(obj);
        TColStd_Array1OfReal weights(1, list.size());
        int index=1;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            weights(index++) = (double)Py::Float(*it);
        }

        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::getWeight(PyObject *args)
{
    int uindex,vindex;
    if (!PyArg_ParseTuple(args, "ii",&uindex,&vindex))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        Standard_OutOfRange_Raise_if
            (uindex < 1 || uindex > surf->NbUPoles() ||
             vindex < 1 || vindex > surf->NbVPoles(), "Weight index out of range");
        double w = surf->Weight(uindex,vindex);
        return Py_BuildValue("d", w);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::getWeights(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::getResolution(PyObject *args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return 0;
    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
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

PyObject* BezierSurfacePy::exchangeUV(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        //FIXME: Crashes
        surf->ExchangeUV();
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::uIso(PyObject * args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d", &u))
        return 0;

    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        Handle_Geom_Curve c = surf->UIso(u);
        return new BezierCurvePy(new GeomBezierCurve(Handle_Geom_BezierCurve::DownCast(c)));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BezierSurfacePy::vIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return 0;

    try {
        Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
            (getGeometryPtr()->handle());
        Handle_Geom_Curve c = surf->VIso(v);
        return new BezierCurvePy(new GeomBezierCurve(Handle_Geom_BezierCurve::DownCast(c)));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

Py::Int BezierSurfacePy::getUDegree(void) const
{
    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->UDegree()); 
}

Py::Int BezierSurfacePy::getVDegree(void) const
{
    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->UDegree()); 
}

Py::Int BezierSurfacePy::getMaxDegree(void) const
{
    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->MaxDegree()); 
}

Py::Int BezierSurfacePy::getNbUPoles(void) const
{
    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->NbUPoles()); 
}

Py::Int BezierSurfacePy::getNbVPoles(void) const
{
    Handle_Geom_BezierSurface surf = Handle_Geom_BezierSurface::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(surf->NbVPoles()); 
}

PyObject *BezierSurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int BezierSurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
