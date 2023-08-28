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
# include <boost/uuid/uuid_io.hpp>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/Matrix.h>
#include <Base/MatrixPy.h>
#include <Base/Placement.h>
#include <Base/PlacementPy.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include "GeometryPy.h"
#include "GeometryPy.cpp"
#include "GeometryExtensionPy.h"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometryPy::representation() const
{
    return "<Geometry object>";
}

PyObject *GeometryPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'Geometry'.");
    return nullptr;
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
        getGeometryPtr()->mirror(vec);
        Py_Return;
    }

    PyErr_Clear();
    PyObject* axis;
    if (PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type),&o,
                                       &(Base::VectorPy::Type),&axis)) {
        Base::Vector3d pnt = static_cast<Base::VectorPy*>(o)->value();
        Base::Vector3d dir = static_cast<Base::VectorPy*>(axis)->value();
        getGeometryPtr()->mirror(pnt, dir);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "either a point (vector) or axis (vector, vector) must be given");
    return nullptr;
}

PyObject* GeometryPy::rotate(PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type),&o))
        return nullptr;

    Base::Placement* plm = static_cast<Base::PlacementPy*>(o)->getPlacementPtr();
    getGeometryPtr()->rotate(*plm);
    Py_Return;
}

PyObject* GeometryPy::scale(PyObject *args)
{
    PyObject* o;
    double scale;
    Base::Vector3d vec;
    if (PyArg_ParseTuple(args, "O!d", &(Base::VectorPy::Type),&o, &scale)) {
        vec = static_cast<Base::VectorPy*>(o)->value();
        getGeometryPtr()->scale(vec, scale);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!d", &PyTuple_Type,&o, &scale)) {
        vec = Base::getVectorFromTuple<double>(o);
        getGeometryPtr()->scale(vec, scale);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "either vector or tuple and float expected");
    return nullptr;
}

PyObject* GeometryPy::transform(PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type),&o))
        return nullptr;
    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(o)->value();
    getGeometryPtr()->transform(mat);
    Py_Return;
}

PyObject* GeometryPy::translate(PyObject *args)
{
    PyObject* o;
    Base::Vector3d vec;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type),&o)) {
        vec = static_cast<Base::VectorPy*>(o)->value();
        getGeometryPtr()->translate(vec);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &PyTuple_Type,&o)) {
        vec = Base::getVectorFromTuple<double>(o);
        getGeometryPtr()->translate(vec);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "either vector or tuple expected");
    return nullptr;
}

PyObject* GeometryPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Part::Geometry* geom = this->getGeometryPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of geometry");
        return nullptr;
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
        return nullptr;

    Part::Geometry* geom = this->getGeometryPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create clone of geometry");
        return nullptr;
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

PyObject* GeometryPy::setExtension(PyObject *args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(GeometryExtensionPy::Type),&o)) {
        Part::GeometryExtension * ext;
        ext = static_cast<GeometryExtensionPy *>(o)->getGeometryExtensionPtr();

        // make copy of Python managed memory and wrap it in smart pointer
        auto cpy = ext->copy();

        this->getGeometryPtr()->setExtension(std::move(cpy));
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "A geometry extension object was expected");
    return nullptr;
}

PyObject* GeometryPy::getExtensionOfType(PyObject *args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        Base::Type type = Base::Type::fromName(o);

        if(type != Base::Type::badType()) {
            try {
                std::shared_ptr<const GeometryExtension> ext(this->getGeometryPtr()->getExtension(type));

                // we create a copy and transfer this copy's memory management responsibility to Python
                PyObject* cpy = ext->copyPyObject();
                return cpy;
            }
            catch(const Base::ValueError& e) {
                PyErr_SetString(PartExceptionOCCError, e.what());
                return nullptr;
            }
            catch(const std::bad_weak_ptr&) {
                PyErr_SetString(PartExceptionOCCError, "Geometry extension does not exist anymore.");
                return nullptr;
            }
            catch(Base::NotImplementedError&) {
                PyErr_SetString(Part::PartExceptionOCCError, "Geometry extension does not implement a Python counterpart.");
                return nullptr;
            }
        }
        else
        {
            PyErr_SetString(PartExceptionOCCError, "Exception type does not exist");
            return nullptr;
        }

    }

    PyErr_SetString(PartExceptionOCCError, "A string with the name of the geometry extension type was expected");
    return nullptr;
}

PyObject* GeometryPy::getExtensionOfName(PyObject *args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        try {
            std::shared_ptr<const GeometryExtension> ext(this->getGeometryPtr()->getExtension(std::string(o)));

            // we create a copy and transfer this copy's memory management responsibility to Python
            PyObject* cpy = ext->copyPyObject();
            return cpy;
        }
        catch(const Base::ValueError& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return nullptr;
        }
        catch(const std::bad_weak_ptr&) {
            PyErr_SetString(PartExceptionOCCError, "Geometry extension does not exist anymore.");
            return nullptr;
        }
        catch(Base::NotImplementedError&) {
            PyErr_SetString(Part::PartExceptionOCCError, "Geometry extension does not implement a Python counterpart.");
            return nullptr;
        }

    }

    PyErr_SetString(PartExceptionOCCError, "A string with the name of the geometry extension was expected");
    return nullptr;
}

PyObject* GeometryPy::hasExtensionOfType(PyObject *args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        Base::Type type = Base::Type::fromName(o);

        if(type != Base::Type::badType()) {
            try {
                return Py::new_reference_to(Py::Boolean(this->getGeometryPtr()->hasExtension(type)));
            }
            catch(const Base::ValueError& e) {
                PyErr_SetString(PartExceptionOCCError, e.what());
                return nullptr;
            }
        }
        else
        {
            PyErr_SetString(PartExceptionOCCError, "Exception type does not exist");
            return nullptr;
        }

    }

    PyErr_SetString(PartExceptionOCCError, "A string with the type of the geometry extension was expected");
    return nullptr;
}

PyObject* GeometryPy::hasExtensionOfName(PyObject *args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        try {
            return Py::new_reference_to(Py::Boolean(this->getGeometryPtr()->hasExtension(std::string(o))));
        }
        catch(const Base::ValueError& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return nullptr;
        }

    }

    PyErr_SetString(PartExceptionOCCError, "A string with the type of the geometry extension was expected");
    return nullptr;
}

PyObject* GeometryPy::deleteExtensionOfType(PyObject *args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        Base::Type type = Base::Type::fromName(o);

        if(type != Base::Type::badType()) {
            try {
                this->getGeometryPtr()->deleteExtension(type);
                Py_Return;
            }
            catch(const Base::ValueError& e) {
                PyErr_SetString(PartExceptionOCCError, e.what());
                return nullptr;
            }
        }
        else
        {
            PyErr_SetString(PartExceptionOCCError, "Type does not exist");
            return nullptr;
        }

    }

    PyErr_SetString(PartExceptionOCCError, "A string with a type object was expected");
    return nullptr;
}

PyObject* GeometryPy::deleteExtensionOfName(PyObject *args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        try {
            this->getGeometryPtr()->deleteExtension(std::string(o));
            Py_Return;
        }
        catch(const Base::ValueError& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return nullptr;
        }
    }

    PyErr_SetString(PartExceptionOCCError, "A string with the name of the extension was expected");
    return nullptr;
}

PyObject* GeometryPy::getExtensions(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")){
        PyErr_SetString(PartExceptionOCCError, "No arguments were expected");
        return nullptr;
    }

    try {
        const std::vector<std::weak_ptr<const GeometryExtension>> ext = this->getGeometryPtr()->getExtensions();

        Py::List list;

        for (const auto & it : ext) {

            // const casting only to get the Python object to make a copy
            std::shared_ptr<GeometryExtension> p = std::const_pointer_cast<GeometryExtension>(it.lock());

            if(p) {
                // we create a python copy and add it to the list

                try {
                    list.append(Py::asObject(p->copyPyObject()));
                }
                catch(Base::NotImplementedError&) {
                    // silently ignoring extensions not having a Python object
                }
            }
        }

        return Py::new_reference_to(list);
    }
    catch(const Base::ValueError& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return nullptr;
    }

}

Py::String GeometryPy::getTag() const
{
    std::string tmp = boost::uuids::to_string(getGeometryPtr()->getTag());
    return {tmp};
}

PyObject *GeometryPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int GeometryPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
