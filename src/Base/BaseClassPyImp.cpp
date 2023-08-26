/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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

// inclusion of the generated files (generated out of BaseClassPy.xml)
#include "BaseClassPy.h"
#include "BaseClassPy.cpp"

using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string BaseClassPy::representation() const
{
    return {"<binding object>"};
}


PyObject*  BaseClassPy::isDerivedFrom(PyObject *args)
{
    char *name{};
    if (!PyArg_ParseTuple(args, "s", &name))
        return nullptr;

    Base::Type type = Base::Type::fromName(name);
    bool valid = (type != Base::Type::badType() && getBaseClassPtr()->getTypeId().isDerivedFrom(type));
    return PyBool_FromLong(valid ? 1 : 0);
}

PyObject*  BaseClassPy::getAllDerivedFrom(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    std::vector<Base::Type> ary;
    Base::Type::getAllDerivedFrom(getBaseClassPtr()->getTypeId(), ary);
    Py::List res;
    for (const auto & it : ary)
        res.append(Py::String(it.getName()));
    return Py::new_reference_to(res);
}

Py::String BaseClassPy::getTypeId() const
{
    return {std::string(getBaseClassPtr()->getTypeId().getName())};
}

Py::String BaseClassPy::getModule() const
{
    std::string module(getBaseClassPtr()->getTypeId().getName());
    std::string::size_type pos = module.find_first_of("::");

    if (pos != std::string::npos)
        module = std::string(module, 0, pos);
    else
        module.clear();

    return {module};
}

PyObject *BaseClassPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BaseClassPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


