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
# include <gp_Ax1.hxx>
# include <gp_Dir.hxx>
# include <gp_Pnt.hxx>
# include <gp_Vec.hxx>
# include <gp_Trsf.hxx>
# include <Geom_Geometry.hxx>
# include <Geom_Curve.hxx>
# include <Geom_Surface.hxx>
# include <Precision.hxx>
# include <Standard_Failure.hxx>

# include <boost/uuid/uuid_io.hpp>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/Matrix.h>
#include <Base/MatrixPy.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>
#include <Base/Rotation.h>
#include <Base/Placement.h>
#include <Base/PlacementPy.h>

#include "OCCError.h"
#include "Geometry.h"
#include "GeometryPy.h"
#include "GeometryPy.cpp"

#include "TopoShape.h"
#include "TopoShapePy.h"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometryPy::representation(void) const
{
    return "<Geometry object>";
}

PyObject *GeometryPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'Geometry'.");
    return 0;
}

// constructor method
int GeometryPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* GeometryPy::mirror(PyObject *args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type),&o)) {
        Base::Vector3d vec = static_cast<Base::VectorPy*>(o)->value();
        gp_Pnt pnt(vec.x, vec.y, vec.z);
        getGeometryPtr()->handle()->Mirror(pnt);
        Py_Return;
    }

    PyErr_Clear();
    PyObject* axis;
    if (PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type),&o,
                                       &(Base::VectorPy::Type),&axis)) {
        Base::Vector3d pnt = static_cast<Base::VectorPy*>(o)->value();
        Base::Vector3d dir = static_cast<Base::VectorPy*>(axis)->value();
        gp_Ax1 ax1(gp_Pnt(pnt.x,pnt.y,pnt.z), gp_Dir(dir.x,dir.y,dir.z));
        getGeometryPtr()->handle()->Mirror(ax1);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "either a point (vector) or axis (vector, vector) must be given");
    return 0;
}

PyObject* GeometryPy::rotate(PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type),&o))
        return 0;

    Base::Placement* plm = static_cast<Base::PlacementPy*>(o)->getPlacementPtr();
    Base::Rotation rot(plm->getRotation());
    Base::Vector3d pnt, dir;
    double angle;

    rot.getValue(dir, angle);
    pnt = plm->getPosition();

    gp_Ax1 ax1(gp_Pnt(pnt.x,pnt.y,pnt.z), gp_Dir(dir.x,dir.y,dir.z));
    getGeometryPtr()->handle()->Rotate(ax1, angle);
    Py_Return;
}

PyObject* GeometryPy::scale(PyObject *args)
{
    PyObject* o;
    double scale;
    Base::Vector3d vec;
    if (PyArg_ParseTuple(args, "O!d", &(Base::VectorPy::Type),&o, &scale)) {
        vec = static_cast<Base::VectorPy*>(o)->value();
        gp_Pnt pnt(vec.x, vec.y, vec.z);
        getGeometryPtr()->handle()->Scale(pnt, scale);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!d", &PyTuple_Type,&o, &scale)) {
        vec = Base::getVectorFromTuple<double>(o);
        gp_Pnt pnt(vec.x, vec.y, vec.z);
        getGeometryPtr()->handle()->Scale(pnt, scale);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "either vector or tuple and float expected");
    return 0;
}

PyObject* GeometryPy::transform(PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type),&o))
        return 0;
    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(o)->value();
    gp_Trsf trf;
    trf.SetValues(mat[0][0],mat[0][1],mat[0][2],mat[0][3],
                  mat[1][0],mat[1][1],mat[1][2],mat[1][3],
                  mat[2][0],mat[2][1],mat[2][2],mat[2][3]
#if OCC_VERSION_HEX < 0x060800
                  , 0.00001,0.00001
#endif
                ); //precision was removed in OCCT CR0025194
    getGeometryPtr()->handle()->Transform(trf);
    Py_Return;
}

PyObject* GeometryPy::translate(PyObject *args)
{
    PyObject* o;
    Base::Vector3d vec;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type),&o)) {
        vec = static_cast<Base::VectorPy*>(o)->value();
        gp_Vec trl(vec.x, vec.y, vec.z);
        getGeometryPtr()->handle()->Translate(trl);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &PyTuple_Type,&o)) {
        vec = Base::getVectorFromTuple<double>(o);
        gp_Vec trl(vec.x, vec.y, vec.z);
        getGeometryPtr()->handle()->Translate(trl);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "either vector or tuple expected");
    return 0;
}

PyObject* GeometryPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Part::Geometry* geom = this->getGeometryPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of geometry");
        return 0;
    }

    Part::GeometryPy* geompy = static_cast<Part::GeometryPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'Geometry' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        Part::Geometry* clone = static_cast<Part::Geometry*>(geompy->_pcTwinPointer);
        delete clone;
    }
    geompy->_pcTwinPointer = geom->copy();
    return cpy;
}

PyObject* GeometryPy::clone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Part::Geometry* geom = this->getGeometryPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create clone of geometry");
        return 0;
    }

    Part::GeometryPy* geompy = static_cast<Part::GeometryPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'Geometry' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        Part::Geometry* clone = static_cast<Part::Geometry*>(geompy->_pcTwinPointer);
        delete clone;
    }
    geompy->_pcTwinPointer = geom->clone();
    return cpy;
}

Py::Boolean GeometryPy::getConstruction(void) const
{
    return Py::Boolean(getGeometryPtr()->Construction);
}

void  GeometryPy::setConstruction(Py::Boolean arg)
{
    if (getGeometryPtr()->getTypeId() != Part::GeomPoint::getClassTypeId())
        getGeometryPtr()->Construction = arg;
}

Py::String GeometryPy::getTag(void) const
{
    std::string tmp = boost::uuids::to_string(getGeometryPtr()->getTag());
    return Py::String(tmp);
}

Py::Int GeometryPy::getID() const {
    return Py::Int(getGeometryPtr()->Id);
}

void GeometryPy::setID(Py::Int pyId) {
    auto id = (long)pyId;
    if(id<=0)
        throw Py::ValueError("geometry id must be positive");
    getGeometryPtr()->Id = id;
}

Py::String GeometryPy::getRef() const {
    return Py::String(getGeometryPtr()->Ref);
}

void GeometryPy::setRef(Py::String ref) {
    getGeometryPtr()->Ref = ref.as_string();
}

Py::Boolean GeometryPy::getDetached() const {
    return Py::Boolean(getGeometryPtr()->testFlag(Part::Geometry::Detached));
}

void GeometryPy::setDetached(Py::Boolean detached) {
    getGeometryPtr()->setFlag(Part::Geometry::Detached,detached);
}

Py::Boolean GeometryPy::getDefining() const {
    return Py::Boolean(getGeometryPtr()->testFlag(Part::Geometry::Defining));
}

void GeometryPy::setDefining(Py::Boolean defining) {
    getGeometryPtr()->setFlag(Part::Geometry::Defining,defining);
}

Py::Boolean GeometryPy::getFrozen() const {
    return Py::Boolean(getGeometryPtr()->testFlag(Part::Geometry::Frozen));
}

void GeometryPy::setFrozen(Py::Boolean frozen) {
    getGeometryPtr()->setFlag(Part::Geometry::Frozen,frozen);
}

PyObject *GeometryPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int GeometryPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
