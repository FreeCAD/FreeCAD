// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
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

#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <Base/Console.h>

#include <Base/PyObjectBase.h>

#include <gsl/pointers>

namespace Start
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Start")
    {
        initialize("This module is the Start module.");  // register with Python
    }
};

PyObject* initModule()
{
    auto newModule = gsl::owner<Module*>(new Module);
    return Base::Interpreter().addModule(newModule);  // Transfer ownership
}

}  // namespace Start

/* Python entry */
PyMOD_INIT_FUNC(Start)
{
    PyObject* mod = Start::initModule();
    Base::Console().log("Loading Start module... done\n");
    PyMOD_Return(mod);
}
