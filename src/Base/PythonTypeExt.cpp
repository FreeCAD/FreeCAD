// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2023 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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
#include <CXX/Extensions.hxx>

#include <Base/PythonTypeExt.h>


using namespace Base;

PythonTypeExt::PythonTypeExt(Py::PythonType& type)
    : pytype(type)
{
}

Py::PythonType& PythonTypeExt::set_tp_descr_get(PyObject* (*tp_descr_get)(PyObject* self, PyObject* obj, PyObject* type))
{
    pytype.type_object()->tp_descr_get = tp_descr_get;

    return pytype;
}

Py::PythonType& PythonTypeExt::set_tp_descr_set(int (*tp_descr_set)(PyObject* self, PyObject* obj, PyObject* value))
{
    pytype.type_object()->tp_descr_set = tp_descr_set;

    return pytype;
}
