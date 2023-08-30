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

#include "PreCompiled.h"
#include <Base/PythonTypeExt.h>

#include "Command.h"
#include "Action.h"
#include "PythonWrapper.h"

#include "CommandActionPy.h"
#include "CommandPy.h"


using namespace Gui;

CommandActionPy::CommandActionPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds)
    : Py::PythonClass<CommandActionPy>::PythonClass(self, args, kwds)
{
    const char* name;
    if (!PyArg_ParseTuple(args.ptr(), "s", &name)) {
        throw Py::Exception();
    }

    cmdName = name;
    cmd = Application::Instance->commandManager().getCommandByName(name);
}

CommandActionPy::~CommandActionPy() = default;

Py::Object CommandActionPy::getAction()
{
    if (!cmd) {
        cmd = Application::Instance->commandManager().getCommandByName(cmdName.c_str());
    }

    Action* action = cmd ? cmd->getAction() : nullptr;
    if (action) {
        PythonWrapper wrap;
        wrap.loadWidgetsModule();

        return wrap.fromQObject(action->action());
    }
    else {
        return Py::None();
    }
}

Py::Object CommandActionPy::getCommand()
{
    if (!cmd) {
        cmd = Application::Instance->commandManager().getCommandByName(cmdName.c_str());
    }

    if (cmd) {
        auto cmdPy = new CommandPy(cmd);
        return Py::asObject(cmdPy);
    }

    return Py::None();
}
PYCXX_NOARGS_METHOD_DECL(CommandActionPy, getCommand)

void CommandActionPy::init_type()
{
    Base::PythonTypeExt ext(behaviors());

    behaviors().name("Gui.CommandAction");
    behaviors().doc("Descriptor to access the action of the commands");
    behaviors().supportRepr();
    behaviors().supportGetattro();
    behaviors().supportSetattro();
    ext.set_tp_descr_get(&CommandActionPy::descriptorGetter);
    ext.set_tp_descr_set(&CommandActionPy::descriptorSetter);
    PYCXX_ADD_NOARGS_METHOD(getCommand, getCommand, "Descriptor associated command");

    behaviors().readyType();
}

PyObject* CommandActionPy::descriptorGetter(PyObject* self, PyObject* /*obj*/, PyObject* /*type*/)
{
    auto cmdAction = Py::PythonClassObject<CommandActionPy>(self).getCxxObject();

    return Py::new_reference_to(cmdAction->getAction());
}

int CommandActionPy::descriptorSetter(PyObject* /*self*/, PyObject* /*obj*/, PyObject* value)
{
    if (value) {
        PyErr_SetString(PyExc_AttributeError, "Can't overwrite command action");
    }
    else {
        PyErr_SetString(PyExc_AttributeError, "Can't delete command action");
    }

    return -1;
}

Py::Object CommandActionPy::repr()
{
    std::stringstream s;
    s << this->cmdName << " command action descriptor";

    return Py::String(s.str());
}

Py::Object CommandActionPy::getattro(const Py::String& attr_)
{
    std::string attr = static_cast<std::string>(attr_);
    Py::Dict d;
    d["name"] = Py::String(this->cmdName);
    if (attr == "__dict__") {
        return d;
    }
    else if (attr == "name") {
        return d["name"];
    }
    else {
        return genericGetAttro(attr_);
    }
}

int CommandActionPy::setattro(const Py::String& attr_, const Py::Object& value)
{
    std::string attr = static_cast<std::string>(attr_);
    if (attr == "name" && value.isString()) {
        cmdName = static_cast<std::string>(Py::String(value));
        cmd = Application::Instance->commandManager().getCommandByName(cmdName.c_str());
    }
    else {
        return genericSetAttro(attr_, value);
    }

    return 0;
}
