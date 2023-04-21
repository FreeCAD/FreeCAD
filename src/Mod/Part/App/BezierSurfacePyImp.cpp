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
# include <Geom_BezierSurface.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColgp_Array2OfPnt.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_Array2OfReal.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "BezierCurvePy.h"
#include "BezierSurfacePy.h"
#include "BezierSurfacePy.cpp"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BezierSurfacePy::representation() const
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
        return nullptr;

    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
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
        return nullptr;

    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsURational();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierSurfacePy::isVRational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVRational();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierSurfacePy::isUPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierSurfacePy::isVPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierSurfacePy::isUClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierSurfacePy::isVClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* BezierSurfacePy::increase(PyObject *args)
{
    int udegree,vdegree;
    if (!PyArg_ParseTuple(args, "ii", &udegree, &vdegree))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->Increase(udegree, vdegree);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::insertPoleColAfter(PyObject *args)
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

        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        if (!obj2) {
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
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::insertPoleRowAfter(PyObject *args)
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

        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        if (!obj2) {
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
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::insertPoleColBefore(PyObject *args)
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

        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        if (!obj2) {
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
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::insertPoleRowBefore(PyObject *args)
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

        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        if (!obj2) {
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
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::removePoleCol(PyObject *args)
{
    int vindex;
    if (!PyArg_ParseTuple(args, "i",&vindex))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->RemovePoleCol(vindex);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::removePoleRow(PyObject *args)
{
    int uindex;
    if (!PyArg_ParseTuple(args, "i",&uindex))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->RemovePoleRow(uindex);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::segment(PyObject *args)
{
    Standard_Real u1,u2,v1,v2;
    if (!PyArg_ParseTuple(args, "dddd",&u1,&u2,&v1,&v2))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->Segment(u1,u2,v1,v2);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::setPole(PyObject *args)
{
    int uindex,vindex;
    PyObject* obj;
    double weight=0.0;
    if (!PyArg_ParseTuple(args, "iiO!|d",&uindex,&vindex,&(Base::VectorPy::Type),&obj,&weight))
        return nullptr;
    try {
        Base::Vector3d pole = static_cast<Base::VectorPy*>(obj)->value();
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        if (weight <= gp::Resolution())
            surf->SetPole(uindex,vindex,gp_Pnt(pole.x,pole.y,pole.z));
        else
            surf->SetPole(uindex,vindex,gp_Pnt(pole.x,pole.y,pole.z),weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::setPoleCol(PyObject *args)
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

        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
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

PyObject* BezierSurfacePy::setPoleRow(PyObject *args)
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

        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
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

PyObject* BezierSurfacePy::getPole(PyObject *args)
{
    int uindex,vindex;
    if (!PyArg_ParseTuple(args, "ii",&uindex,&vindex))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        Standard_OutOfRange_Raise_if
            (uindex < 1 || uindex > surf->NbUPoles() ||
             vindex < 1 || vindex > surf->NbVPoles(), "Pole index out of range");
        gp_Pnt p = surf->Pole(uindex,vindex);
        return new Base::VectorPy(Base::Vector3d(p.X(),p.Y(),p.Z()));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::getPoles(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
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

PyObject* BezierSurfacePy::setWeight(PyObject *args)
{
    int uindex,vindex;
    double weight;
    if (!PyArg_ParseTuple(args, "iid",&uindex,&vindex,&weight))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetWeight(uindex,vindex,weight);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::setWeightCol(PyObject *args)
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

        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetWeightCol(vindex, weights);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::setWeightRow(PyObject *args)
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

        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        surf->SetWeightRow(uindex, weights);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BezierSurfacePy::getWeight(PyObject *args)
{
    int uindex,vindex;
    if (!PyArg_ParseTuple(args, "ii",&uindex,&vindex))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
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

PyObject* BezierSurfacePy::getWeights(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
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

PyObject* BezierSurfacePy::getResolution(PyObject *args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return nullptr;
    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
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

PyObject* BezierSurfacePy::exchangeUV(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
            (getGeometryPtr()->handle());
        //FIXME: Crashes
        surf->ExchangeUV();
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

Py::Long BezierSurfacePy::getUDegree() const
{
    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->UDegree());
}

Py::Long BezierSurfacePy::getVDegree() const
{
    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->VDegree());
}

Py::Long BezierSurfacePy::getMaxDegree() const
{
    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->MaxDegree());
}

Py::Long BezierSurfacePy::getNbUPoles() const
{
    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->NbUPoles());
}

Py::Long BezierSurfacePy::getNbVPoles() const
{
    Handle(Geom_BezierSurface) surf = Handle(Geom_BezierSurface)::DownCast
        (getGeometryPtr()->handle());
    return Py::Long(surf->NbVPoles());
}

PyObject *BezierSurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BezierSurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
