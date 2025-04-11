// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include <Base/Interpreter.h>

#include "PostDataObjectPy.h"


using namespace Fem;

PostDataObjectPy::PostDataObjectPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds)
    : Py::PythonClass<PostDataObjectPy>::PythonClass(self, args, kwds)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
}

PostDataObjectPy::~PostDataObjectPy() = default;

Py::Object PostDataObjectPy::getDescription()
{
    if (data) {
        std::stringstream ss;
        data->Print(ss);
        Py::String descr = ss.str();
        return descr;
    }

    return Py::None();
}
PYCXX_NOARGS_METHOD_DECL(PostDataObjectPy, getDescription)

void PostDataObjectPy::init_type(PyObject* mod)
{
    behaviors().name("Fem.PostDataObject");
    behaviors().doc("VTK data object");
    behaviors().supportRepr();
    behaviors().supportGetattro();
    PYCXX_ADD_NOARGS_METHOD(getDescription, getDescription, "VTK data object Description");

    behaviors().readyType();

    Base::Interpreter().addType(type_object(), mod, "PostDataObject");
}

Py::Object PostDataObjectPy::repr()
{
    if (!data) {
        return Py::String("");
    }
    std::stringstream s;
    s << "<" << data->GetClassName() << " at " << data.GetPointer() << ">";

    return Py::String(s.str());
}

Py::Object PostDataObjectPy::getattro(const Py::String& attr_)
{
    std::string attr = static_cast<std::string>(attr_);
    Py::Dict d;
    if (attr == "__dict__") {
        return d;
    }
    return genericGetAttro(attr_);
}

Py::Object PostDataObjectPy::create(vtkSmartPointer<vtkDataObject> dataObj)
{
    Py::Callable class_type(type());
    Py::PythonClassObject<PostDataObjectPy> classObj(class_type.apply(Py::Tuple(), Py::Dict()));
    classObj.getCxxObject()->data = dataObj;

    return classObj;
}
