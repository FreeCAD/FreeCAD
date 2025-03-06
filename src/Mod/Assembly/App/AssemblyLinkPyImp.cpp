// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#include "PreCompiled.h"

// inclusion of the generated files (generated out of AssemblyLink.xml)
#include "AssemblyLinkPy.h"
#include "AssemblyLinkPy.cpp"

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string AssemblyLinkPy::representation() const
{
    return {"<Assembly link>"};
}

PyObject* AssemblyLinkPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int AssemblyLinkPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

Py::List AssemblyLinkPy::getJoints() const
{
    Py::List ret;
    std::vector<App::DocumentObject*> list = getAssemblyLinkPtr()->getJoints();

    for (auto It : list) {
        ret.append(Py::Object(It->getPyObject(), true));
    }

    return ret;
}
