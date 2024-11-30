/***************************************************************************
 *   Copyright (c) 2020 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include "ExternalGeometryFacadePy.h"

#include "ExternalGeometryFacadePy.cpp"


using namespace Sketcher;

// returns a string which represents the object e.g. when printed in python
std::string ExternalGeometryFacadePy::representation() const
{
    std::stringstream str;
    str << "<ExternalGeometryFacadePy ( Id=";

    str << getExternalGeometryFacadePtr()->getId() << " ) >";
    return str.str();
}

PyObject* ExternalGeometryFacadePy::PyMake(struct _typeobject*,
                                           PyObject*,
                                           PyObject*)  // Python wrapper
{
    // create a new instance of PointPy and the Twin object
    return new ExternalGeometryFacadePy(new ExternalGeometryFacade());
}

// constructor method
int ExternalGeometryFacadePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* object;
    if (PyArg_ParseTuple(args, "O!", &(Part::GeometryPy::Type), &object)) {
        Part::Geometry* geo = static_cast<Part::GeometryPy*>(object)->getGeometryPtr();

        getExternalGeometryFacadePtr()->setGeometry(geo->clone());

        return 0;
    }

    PyErr_SetString(PyExc_TypeError,
                    "Sketcher::ExternalGeometryFacade constructor accepts:\n"
                    "-- Part.Geometry\n");
    return -1;
}

PyObject* ExternalGeometryFacadePy::testFlag(PyObject* args)
{
    char* flag;
    if (PyArg_ParseTuple(args, "s", &flag)) {

        auto pos = std::find_if(ExternalGeometryExtension::flag2str.begin(),
                                ExternalGeometryExtension::flag2str.end(),
                                [flag](const char* val) {
                                    return strcmp(val, flag) == 0;
                                });

        if (pos != ExternalGeometryExtension::flag2str.end()) {
            int index = std::distance(ExternalGeometryExtension::flag2str.begin(), pos);

            return new_reference_to(
                Py::Boolean(this->getExternalGeometryFacadePtr()->testFlag(index)));
        }

        PyErr_SetString(PyExc_TypeError, "Flag string does not exist.");
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "No flag string provided.");
    return nullptr;
}

PyObject* ExternalGeometryFacadePy::setFlag(PyObject* args)
{
    char* flag;
    PyObject* bflag = Py_True;
    if (PyArg_ParseTuple(args, "s|O!", &flag, &PyBool_Type, &bflag)) {

        auto pos = std::find_if(ExternalGeometryExtension::flag2str.begin(),
                                ExternalGeometryExtension::flag2str.end(),
                                [flag](const char* val) {
                                    return strcmp(val, flag) == 0;
                                });

        if (pos != ExternalGeometryExtension::flag2str.end()) {
            int index = std::distance(ExternalGeometryExtension::flag2str.begin(), pos);

            this->getExternalGeometryFacadePtr()->setFlag(index, Base::asBoolean(bflag));
            Py_Return;
        }

        PyErr_SetString(PyExc_TypeError, "Flag string does not exist.");
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "No flag string provided.");
    Py_Return;
}

Py::String ExternalGeometryFacadePy::getRef() const
{
    return Py::String(this->getExternalGeometryFacadePtr()->getRef());
}

void ExternalGeometryFacadePy::setRef(Py::String value)
{
    this->getExternalGeometryFacadePtr()->setRef(value.as_std_string());
}

Py::Long ExternalGeometryFacadePy::getId() const
{
    return Py::Long(this->getExternalGeometryFacadePtr()->getId());
}

void ExternalGeometryFacadePy::setId(Py::Long Id)
{
    this->getExternalGeometryFacadePtr()->setId(long(Id));
}

Py::String ExternalGeometryFacadePy::getInternalType() const
{
    int internaltypeindex = (int)this->getExternalGeometryFacadePtr()->getInternalType();

    if (internaltypeindex >= InternalType::NumInternalGeometryType) {
        throw Py::NotImplementedError("String name of enum not implemented");
    }

    std::string typestr = SketchGeometryExtension::internaltype2str[internaltypeindex];

    return Py::String(typestr);
}

void ExternalGeometryFacadePy::setInternalType(Py::String arg)
{
    std::string argstr = arg;
    InternalType::InternalType type;

    if (SketchGeometryExtension::getInternalTypeFromName(argstr, type)) {
        this->getExternalGeometryFacadePtr()->setInternalType(type);
        return;
    }

    throw Py::ValueError("Argument is not a valid internal geometry type.");
}

Py::Boolean ExternalGeometryFacadePy::getBlocked() const
{
    return Py::Boolean(getExternalGeometryFacadePtr()->getBlocked());
}

void ExternalGeometryFacadePy::setBlocked(Py::Boolean arg)
{
    getExternalGeometryFacadePtr()->setBlocked(arg);
}

PyObject* ExternalGeometryFacadePy::mirror(PyObject* args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &o)) {
        Base::Vector3d vec = static_cast<Base::VectorPy*>(o)->value();
        getExternalGeometryFacadePtr()->mirror(vec);
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
        getExternalGeometryFacadePtr()->mirror(pnt, dir);
        Py_Return;
    }

    PyErr_SetString(Part::PartExceptionOCCError,
                    "either a point (vector) or axis (vector, vector) must be given");
    return nullptr;
}

PyObject* ExternalGeometryFacadePy::rotate(PyObject* args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type), &o)) {
        return nullptr;
    }

    Base::Placement* plm = static_cast<Base::PlacementPy*>(o)->getPlacementPtr();
    getExternalGeometryFacadePtr()->rotate(*plm);
    Py_Return;
}

PyObject* ExternalGeometryFacadePy::scale(PyObject* args)
{
    PyObject* o;
    double scale;
    Base::Vector3d vec;
    if (PyArg_ParseTuple(args, "O!d", &(Base::VectorPy::Type), &o, &scale)) {
        vec = static_cast<Base::VectorPy*>(o)->value();
        getExternalGeometryFacadePtr()->scale(vec, scale);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!d", &PyTuple_Type, &o, &scale)) {
        vec = Base::getVectorFromTuple<double>(o);
        getExternalGeometryFacadePtr()->scale(vec, scale);
        Py_Return;
    }

    PyErr_SetString(Part::PartExceptionOCCError, "either vector or tuple and float expected");
    return nullptr;
}

PyObject* ExternalGeometryFacadePy::transform(PyObject* args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type), &o)) {
        return nullptr;
    }
    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(o)->value();
    getExternalGeometryFacadePtr()->transform(mat);
    Py_Return;
}

PyObject* ExternalGeometryFacadePy::translate(PyObject* args)
{
    PyObject* o;
    Base::Vector3d vec;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &o)) {
        vec = static_cast<Base::VectorPy*>(o)->value();
        getExternalGeometryFacadePtr()->translate(vec);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &PyTuple_Type, &o)) {
        vec = Base::getVectorFromTuple<double>(o);
        getExternalGeometryFacadePtr()->translate(vec);
        Py_Return;
    }

    PyErr_SetString(Part::PartExceptionOCCError, "either vector or tuple expected");
    return nullptr;
}

PyObject* ExternalGeometryFacadePy::setExtension(PyObject* args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(Part::GeometryExtensionPy::Type), &o)) {
        Part::GeometryExtension* ext;
        ext = static_cast<Part::GeometryExtensionPy*>(o)->getGeometryExtensionPtr();

        // make copy of Python managed memory and wrap it in smart pointer
        auto cpy = ext->copy();

        this->getExternalGeometryFacadePtr()->setExtension(std::move(cpy));
        Py_Return;
    }

    PyErr_SetString(Part::PartExceptionOCCError, "A geometry extension object was expected");
    return nullptr;
}

PyObject* ExternalGeometryFacadePy::getExtensionOfType(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        Base::Type type = Base::Type::fromName(o);

        if (type != Base::Type::badType()) {
            try {
                std::shared_ptr<const Part::GeometryExtension> ext(
                    this->getExternalGeometryFacadePtr()->getExtension(type));

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

PyObject* ExternalGeometryFacadePy::getExtensionOfName(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        try {
            std::shared_ptr<const Part::GeometryExtension> ext(
                this->getExternalGeometryFacadePtr()->getExtension(std::string(o)));

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

PyObject* ExternalGeometryFacadePy::hasExtensionOfType(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        Base::Type type = Base::Type::fromName(o);

        if (type != Base::Type::badType()) {
            try {
                return Py::new_reference_to(
                    Py::Boolean(this->getExternalGeometryFacadePtr()->hasExtension(type)));
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

PyObject* ExternalGeometryFacadePy::hasExtensionOfName(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        try {
            return Py::new_reference_to(
                Py::Boolean(this->getExternalGeometryFacadePtr()->hasExtension(std::string(o))));
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

PyObject* ExternalGeometryFacadePy::deleteExtensionOfType(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        Base::Type type = Base::Type::fromName(o);

        if (type != Base::Type::badType()) {
            try {
                this->getExternalGeometryFacadePtr()->deleteExtension(type);
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

PyObject* ExternalGeometryFacadePy::deleteExtensionOfName(PyObject* args)
{
    char* o;
    if (PyArg_ParseTuple(args, "s", &o)) {

        try {
            this->getExternalGeometryFacadePtr()->deleteExtension(std::string(o));
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

PyObject* ExternalGeometryFacadePy::getExtensions(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        PyErr_SetString(Part::PartExceptionOCCError, "No arguments were expected");
        return nullptr;
    }

    try {
        const std::vector<std::weak_ptr<const Part::GeometryExtension>> ext =
            this->getExternalGeometryFacadePtr()->getExtensions();

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

Py::Boolean ExternalGeometryFacadePy::getConstruction() const
{
    return Py::Boolean(getExternalGeometryFacadePtr()->getConstruction());
}

void ExternalGeometryFacadePy::setConstruction(Py::Boolean arg)
{
    if (getExternalGeometryFacadePtr()->getTypeId() != Part::GeomPoint::getClassTypeId()) {
        getExternalGeometryFacadePtr()->setConstruction(arg);
    }
}

Py::Long ExternalGeometryFacadePy::getGeometryLayerId() const
{
    return Py::Long(this->getExternalGeometryFacadePtr()->getGeometryLayerId());
}

void ExternalGeometryFacadePy::setGeometryLayerId(Py::Long Id)
{
    this->getExternalGeometryFacadePtr()->setGeometryLayerId(long(Id));
}

Py::String ExternalGeometryFacadePy::getTag() const
{
    std::string tmp = boost::uuids::to_string(getExternalGeometryFacadePtr()->getTag());
    return Py::String(tmp);
}

Py::Object ExternalGeometryFacadePy::getGeometry() const
{
    // We return a clone
    std::unique_ptr<Part::Geometry> geo(getExternalGeometryFacadePtr()->getGeometry()->clone());
    return Py::Object(geo->getPyObject(), true);
}

void ExternalGeometryFacadePy::setGeometry(Py::Object arg)
{
    if (PyObject_TypeCheck(arg.ptr(), &(Part::GeometryPy::Type))) {
        Part::GeometryPy* gp = static_cast<Part::GeometryPy*>(arg.ptr());

        getExternalGeometryFacadePtr()->setGeometry(gp->getGeometryPtr()->clone());
    }
}


PyObject* ExternalGeometryFacadePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ExternalGeometryFacadePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
