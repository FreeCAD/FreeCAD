// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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
#ifndef _PreComp_
#include <Inventor/nodes/SoSeparator.h>
#endif

#include <Base/Interpreter.h>

#include "ViewProviderFemConstraintPy.h"
#include "ViewProviderFemConstraintPy.cpp"


using namespace FemGui;

// returns a string which represent the object e.g. when printed in python
std::string ViewProviderFemConstraintPy::representation() const
{
    std::stringstream str;
    str << "<View provider FemConstraint object at " << getViewProviderFemConstraintPtr() << ">";

    return str.str();
}

PyObject* ViewProviderFemConstraintPy::loadSymbol(PyObject* args)
{
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    getViewProviderFemConstraintPtr()->loadSymbol(name);

    Py_Return;
}

Py::Object ViewProviderFemConstraintPy::getSymbolNode() const
{
    try {
        SoSeparator* sep = getViewProviderFemConstraintPtr()->getSymbolSeparator();
        if (sep) {
            PyObject* Ptr =
                Base::Interpreter().createSWIGPointerObj("pivy.coin", "_p_SoSeparator", sep, 1);
            sep->ref();

            return Py::Object(Ptr, true);
        }
        else {
            return Py::None();
        }
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object ViewProviderFemConstraintPy::getExtraSymbolNode() const
{
    try {
        SoSeparator* sep = getViewProviderFemConstraintPtr()->getExtraSymbolSeparator();
        if (sep) {
            PyObject* Ptr =
                Base::Interpreter().createSWIGPointerObj("pivy.coin", "_p_SoSeparator", sep, 1);
            sep->ref();

            return Py::Object(Ptr, true);
        }
        else {
            return Py::None();
        }
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Boolean ViewProviderFemConstraintPy::getRotateSymbol() const
{
    return Py::Boolean(getViewProviderFemConstraintPtr()->getRotateSymbol());
}

void ViewProviderFemConstraintPy::setRotateSymbol(Py::Boolean arg)
{
    getViewProviderFemConstraintPtr()->setRotateSymbol((arg));
}

PyObject* ViewProviderFemConstraintPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderFemConstraintPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
