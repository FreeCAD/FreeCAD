// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "Features.h"


namespace SheetMetalCore
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("SheetMetalCore")
    {
        initialize("This module is the SheetMetalCore module.");  // register with Python
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace SheetMetalCore

/* Python entry */
PyMOD_INIT_FUNC(SheetMetalCore)
{
    try {
        Base::Interpreter().runString("import Part");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = SheetMetalCore::initModule();
    Base::Console().log("Loading SheetMetalCore module… done\n");

    // clang-format off
    SheetMetalCore::Feature     ::init();
    SheetMetalCore::BasePanel   ::init();
    SheetMetalCore::FromFlat    ::init();
    SheetMetalCore::FromSolid   ::init();
    SheetMetalCore::Flange      ::init();
    SheetMetalCore::Cut         ::init();
    SheetMetalCore::FlatPattern ::init();
    // clang-format on

    PyMOD_Return(mod);
}
