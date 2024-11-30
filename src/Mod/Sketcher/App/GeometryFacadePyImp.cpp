/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <Base/GeometryPyCXX.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>
#include <Mod/Part/App/GeometryExtensionPy.h>
#include <Mod/Part/App/GeometryPy.h>
#include <Mod/Part/App/OCCError.h>

#include "GeometryFacadePy.h"

#include "GeometryFacadePy.cpp"


using namespace Sketcher;

// returns a string which represents the object e.g. when printed in python
std::string GeometryFacadePy::representation() const
{
    std::stringstream str;
    str << "<GeometryFacade ( Id=";

    str << getGeometryFacadePtr()->getId() << " ) >";
    return str.str();
}

PyObject* GeometryFacadePy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of PointPy and the Twin object
    return new GeometryFacadePy(new GeometryFacade());
}

// constructor method
int GeometryFacadePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* object;
    if (PyArg_ParseTuple(args, "O!", &(Part::GeometryPy::Type), &object)) {
        Part::Geometry* geo = static_cast<Part::GeometryPy*>(object)->getGeometryPtr();

        getGeometryFacadePtr()->setGeometry(geo->clone());

        return 0;
    }

    PyErr_SetString(PyExc_TypeError,
                    "Sketcher::GeometryFacade constructor accepts:\n"
                    "-- Part.Geometry\n");
    return -1;
}

Py::Long GeometryFacadePy::getId() const
{
    return Py::Long(this->getGeometryFacadePtr()->getId());
}

void GeometryFacadePy::setId(Py::Long Id)
{
    this->getGeometryFacadePtr()->setId(long(Id));
}

Py::String GeometryFacadePy::getInternalType() const
{
    int internaltypeindex = (int)this->getGeometryFacadePtr()->getInternalType();

    if (internaltypeindex >= InternalType::NumInternalGeometryType) {
        throw Py::NotImplementedError("String name of enum not implemented");
    }

    std::string typestr = SketchGeometryExtension::internaltype2str[internaltypeindex];

    return Py::String(typestr);
}

void GeometryFacadePy::setInternalType(Py::String arg)
{
    std::string argstr = arg;
    InternalType::InternalType type;

    if (SketchGeometryExtension::getInternalTypeFromName(argstr, type)) {
        this->getGeometryFacadePtr()->setInternalType(type);
        return;
    }

    throw Py::ValueError("Argument is not a valid internal geometry type.");
}

Py::Boolean GeometryFacadePy::getBlocked() const
{
    return Py::Boolean(getGeometryFacadePtr()->getBlocked());
}

void GeometryFacadePy::setBlocked(Py::Boolean arg)
{
    getGeometryFacadePtr()->setBlocked(arg);
}

PyObject* GeometryFacadePy::testGeometryMode(PyObject* args)
{
    char* flag;
    if (PyArg_ParseTuple(args, "s", &flag)) {

        GeometryMode::GeometryMode mode;

        if (SketchGeometryExtension::getGeometryModeFromName(flag, mode)) {
            return new_reference_to(Py::Boolean(getGeometryFacadePtr()->testGeometryMode(mode)));
        }

        PyErr_SetString(PyExc_TypeError, "Flag string does not exist.");
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "No flag string provided.");
    return nullptr;
}

PyObject* GeometryFacadePy::setGeometryMode(PyObject* args)
{
    char* flag;
    PyObject* bflag = Py_True;
    if (PyArg_ParseTuple(args, "s|O!", &flag, &PyBool_Type, &bflag)) {

        GeometryMode::GeometryMode mode;

        if (SketchGeometryExtension::getGeometryModeFromName(flag, mode)) {
            getGeometryFacadePtr()->setGeometryMode(mode, Base::asBoolean(bflag));
            Py_Return;
        }

        PyErr_SetString(PyExc_TypeError, "Flag string does not exist.");
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "No flag string provided.");
    Py_Return;
}


PyObject* GeometryFacadePy::mirror(PyObject* args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &o)) {
        Base::Vector3d vec = static_cast<Base::VectorPy*>(o)->value();
        getGeometryFacadePtr()->mirror(vec);
        Py_Return;
    }

    PyErr_Clear();
    PyObject* axis;
    if (PyArg_ParseTuple(args,
                         "O!O!",
                         &(Base::VectorPy::Type),
                         &o,
                         &(Base::VectorPy::Type),
                         &axis)) {
        Base::Vector3d pnt = static_cast<Base::VectorPy*>(o)->value();
        Base::Vector3d dir = static_cast<Base::VectorPy*>(axis)->value();
        getGeometryFacadePtr()->mirror(pnt, dir);
        Py_Return;
    }

    PyErr_SetString(Part::PartExceptionOCCError,
                    "either a point (vector) or axis (vector, vector) must be given");
    return nullptr;
}

PyObject* GeometryFacadePy::rotate(PyObject* args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type), &o)) {
        return nullptr;
    }

    Base::Placement* plm = static_cast<Base::PlacementPy*>(o)->getPlacementPtr();
    getGeometryFacadePtr()->rotate(*plm);
    Py_Return;
}

PyObject* GeometryFacadePy::scale(PyObject* args)
{
    PyObject* o;
    double scale;
    Base::Vector3d vec;
    if (PyArg_ParseTuple(args, "O!d", &(Base::VectorPy::Type), &o, &scale)) {
        vec = static_cast<Base::VectorPy*>(o)->value();
        getGeometryFacadePtr()->scale(vec, scale);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!d", &PyTuple_Type, &o, &scale)) {
        vec = Base::getVectorFromTuple<double>(o);
        getGeometryFacadePtr()->scale(vec, scale);
        Py_Return;
    }

    PyErr_SetString(Part::PartExceptionOCCError, "either vector or tuple and float expected");
    return nullptr;
}

PyObject* GeometryFacadePy::transform(PyObject* args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type), &o)) {
        return nullptr;
    }
    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(o)->value();
    getGeometryFacadePtr()->transform(mat);
    Py_Return;
}

PyObject* GeometryFacadePy::translate(PyObject* args)
{
    PyObject* o;
    Base::Vector3d vec;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &o)) {
        vec = static_cast<Base::VectorPy*>(o)->value();
        getGeometryFacadePtr()->translate(vec);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &PyTuple_Type, &o)) {
        vec = Base::getVectorFromTuple<double>(o);
        getGeometryFacadePtr()->translate(vec);
        Py_Return;
    }

    PyErr_SetString(Part::PartExceptionOCCError, "either vector or tuple expected");
    return nullptr;
}

PyObject* GeometryFacadePy::setExtension(PyObject* args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(Part::GeometryExtensionPy::Type), &o)) {
        Part::GeometryExtension* ext;
        ext = static_cast<Part::GeometryExtensionPy*>(o)->getGeometryExtensionPtr();

        // make copy of Python managed memory and wrap it in smart pointer
        auto cpy = ext->copy();

        this->getGeometryFacadePtr()->setExtension(std::move(cpy));
        Py_Return;
    }

    PyErr_SetString(Part::PartExceptionOCCError, "A geometry extension object was expected");
    return nullptr;
}

PyObject* GeometryFacadePy::getExtensionOfType(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        Base::Type type = Base::Type::fromName(o);

        if (type != Base::Type::badType()) {
            try {
                std::shared_ptr<const Part::GeometryExtension> ext(
                    this->getGeometryFacadePtr()->getExtension(type));

                // we create a copy and transfer this copy's memory management responsibility to
                // Python
                PyObject* cpy = ext->copyPyObject();
                return cpy;
            }
            catch (const Base::ValueError& e) {
                PyErr_SetString(Part::PartExceptionOCCError, e.what());
                return nullptr;
            }
            catch (const std::bad_weak_ptr&) {
                PyErr_SetString(Part::PartExceptionOCCError,
                                "Geometry extension does not exist anymore.");
                return nullptr;
            }
            catch (Base::NotImplementedError&) {
                PyErr_SetString(Part::PartExceptionOCCError,
                                "Geometry extension does not implement a Python counterpart.");
                return nullptr;
            }
        }
        else {
            PyErr_SetString(Part::PartExceptionOCCError, "Exception type does not exist");
            return nullptr;
        }
    }

    PyErr_SetString(Part::PartExceptionOCCError,
                    "A string with the name of the geometry extension type was expected");
    return nullptr;
}

PyObject* GeometryFacadePy::getExtensionOfName(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        try {
            std::shared_ptr<const Part::GeometryExtension> ext(
                this->getGeometryFacadePtr()->getExtension(std::string(o)));

            // we create a copy and transfer this copy's memory management responsibility to Python
            PyObject* cpy = ext->copyPyObject();
            return cpy;
        }
        catch (const Base::ValueError& e) {
            PyErr_SetString(Part::PartExceptionOCCError, e.what());
            return nullptr;
        }
        catch (const std::bad_weak_ptr&) {
            PyErr_SetString(Part::PartExceptionOCCError,
                            "Geometry extension does not exist anymore.");
            return nullptr;
        }
        catch (Base::NotImplementedError&) {
            PyErr_SetString(Part::PartExceptionOCCError,
                            "Geometry extension does not implement a Python counterpart.");
            return nullptr;
        }
    }

    PyErr_SetString(Part::PartExceptionOCCError,
                    "A string with the name of the geometry extension was expected");
    return nullptr;
}

PyObject* GeometryFacadePy::hasExtensionOfType(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        Base::Type type = Base::Type::fromName(o);

        if (type != Base::Type::badType()) {
            try {
                return Py::new_reference_to(
                    Py::Boolean(this->getGeometryFacadePtr()->hasExtension(type)));
            }
            catch (const Base::ValueError& e) {
                PyErr_SetString(Part::PartExceptionOCCError, e.what());
                return nullptr;
            }
        }
        else {
            PyErr_SetString(Part::PartExceptionOCCError, "Exception type does not exist");
            return nullptr;
        }
    }

    PyErr_SetString(Part::PartExceptionOCCError,
                    "A string with the type of the geometry extension was expected");
    return nullptr;
}

PyObject* GeometryFacadePy::hasExtensionOfName(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        try {
            return Py::new_reference_to(
                Py::Boolean(this->getGeometryFacadePtr()->hasExtension(std::string(o))));
        }
        catch (const Base::ValueError& e) {
            PyErr_SetString(Part::PartExceptionOCCError, e.what());
            return nullptr;
        }
    }

    PyErr_SetString(Part::PartExceptionOCCError,
                    "A string with the type of the geometry extension was expected");
    return nullptr;
}

PyObject* GeometryFacadePy::deleteExtensionOfType(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        Base::Type type = Base::Type::fromName(o);

        if (type != Base::Type::badType()) {
            try {
                this->getGeometryFacadePtr()->deleteExtension(type);
                Py_Return;
            }
            catch (const Base::ValueError& e) {
                PyErr_SetString(Part::PartExceptionOCCError, e.what());
                return nullptr;
            }
        }
        else {
            PyErr_SetString(Part::PartExceptionOCCError, "Type does not exist");
            return nullptr;
        }
    }

    PyErr_SetString(Part::PartExceptionOCCError, "A string with a type object was expected");
    return nullptr;
}

PyObject* GeometryFacadePy::deleteExtensionOfName(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        try {
            this->getGeometryFacadePtr()->deleteExtension(std::string(o));
            Py_Return;
        }
        catch (const Base::ValueError& e) {
            PyErr_SetString(Part::PartExceptionOCCError, e.what());
            return nullptr;
        }
    }

    PyErr_SetString(Part::PartExceptionOCCError,
                    "A string with the name of the extension was expected");
    return nullptr;
}

PyObject* GeometryFacadePy::getExtensions(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        PyErr_SetString(Part::PartExceptionOCCError, "No arguments were expected");
        return nullptr;
    }

    try {
        const std::vector<std::weak_ptr<const Part::GeometryExtension>> ext =
            this->getGeometryFacadePtr()->getExtensions();

        Py::List list;

        for (std::size_t i = 0; i < ext.size(); ++i) {

            std::shared_ptr<const Part::GeometryExtension> p = ext[i].lock();

            if (p) {
                // we create a python copy and add it to the list
                try {
                    list.append(Py::asObject(p->copyPyObject()));
                }
                catch (Base::NotImplementedError&) {
                    // silently ignoring extensions not having a Python object
                }
            }
        }

        return Py::new_reference_to(list);
    }
    catch (const Base::ValueError& e) {
        PyErr_SetString(Part::PartExceptionOCCError, e.what());
        return nullptr;
    }
}

Py::Boolean GeometryFacadePy::getConstruction() const
{
    return Py::Boolean(getGeometryFacadePtr()->getConstruction());
}

void GeometryFacadePy::setConstruction(Py::Boolean arg)
{
    getGeometryFacadePtr()->setConstruction(arg);
}

Py::Long GeometryFacadePy::getGeometryLayerId() const
{
    return Py::Long(this->getGeometryFacadePtr()->getGeometryLayerId());
}

void GeometryFacadePy::setGeometryLayerId(Py::Long Id)
{
    this->getGeometryFacadePtr()->setGeometryLayerId(long(Id));
}

Py::String GeometryFacadePy::getTag() const
{
    std::string tmp = boost::uuids::to_string(getGeometryFacadePtr()->getTag());
    return Py::String(tmp);
}

Py::Object GeometryFacadePy::getGeometry() const
{
    // We return a clone
    std::unique_ptr<Part::Geometry> geo(getGeometryFacadePtr()->getGeometry()->clone());
    return Py::Object(geo->getPyObject(), true);
}

void GeometryFacadePy::setGeometry(Py::Object arg)
{
    if (PyObject_TypeCheck(arg.ptr(), &(Part::GeometryPy::Type))) {
        Part::GeometryPy* gp = static_cast<Part::GeometryPy*>(arg.ptr());

        getGeometryFacadePtr()->setGeometry(gp->getGeometryPtr()->clone());
    }
}


PyObject* GeometryFacadePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int GeometryFacadePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
