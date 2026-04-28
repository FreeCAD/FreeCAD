// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
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

#pragma once

#include <CXX/Extensions.hxx>


namespace Gui
{
class Command;

class CommandActionPy: public Py::PythonClass<CommandActionPy>
{
public:
    static void init_type();

    CommandActionPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kdws);
    ~CommandActionPy() override;

    Py::Object getCommand();

protected:
    static PyObject* descriptorGetter(PyObject* self, PyObject* obj, PyObject* type);
    static int descriptorSetter(PyObject* self, PyObject* obj, PyObject* value);
    Py::Object repr() override;
    Py::Object getattro(const Py::String& attr) override;
    int setattro(const Py::String& attr_, const Py::Object& value) override;

    Py::Object getAction();

private:
    std::string cmdName;
    Command* cmd = nullptr;
};

}  // namespace Gui
