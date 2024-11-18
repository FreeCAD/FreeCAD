
/***************************************************************************
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

#include "AttachExtensionPy.h"
#include "AttachExtensionPy.cpp"
#include "AttachEnginePy.h"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string AttachExtensionPy::representation() const
{
    return {"<Part::AttachableObject>"};
}

PyObject* AttachExtensionPy::positionBySupport(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool bAttached = false;
    try{
        bAttached = this->getAttachExtensionPtr()->positionBySupport();
    } catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    } catch (Base::Exception &e) {
        e.setPyException();
        return nullptr;
    }
    return Py::new_reference_to(Py::Boolean(bAttached));
}

PyObject* AttachExtensionPy::changeAttacherType(PyObject *args)
{
    const char* typeName;
    if (!PyArg_ParseTuple(args, "s", &typeName))
        return nullptr;
    bool ret;
    try{
        ret = this->getAttachExtensionPtr()->changeAttacherType(typeName);
    } catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    } catch (Base::Exception &e) {
        e.setPyException();
        return nullptr;
    }
    return Py::new_reference_to(Py::Boolean(ret));
}

Py::Object AttachExtensionPy::getAttacher() const
{
    try {
        this->getAttachExtensionPtr()->attacher(); //throws if attacher is not set
    } catch (Base::Exception&) {
        return Py::None();
    }

    try {
        return Py::Object( new Attacher::AttachEnginePy(this->getAttachExtensionPtr()->attacher().copy()), true);
    } catch (Standard_Failure& e) {
        throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());
    } catch (Base::Exception &e) {
        e.setPyException();
        throw Py::Exception();
    }

}

PyObject *AttachExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int AttachExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


