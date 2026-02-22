// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 Morten Vajhøj                                                     *
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
#include <Base/PyObjectBase.h>

#include <Gui/Application.h>

#include "ViewProviderMassPropertiesResult.h"

void CreateMassPropertiesCommands();

namespace MassPropertiesGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("MassPropertiesGui")
    {
        initialize("This is the MassPropertiesGui module");
    }

    ~Module() override = default;
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace MassPropertiesGui

PyMOD_INIT_FUNC(MassPropertiesGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    try {
        Base::Interpreter().loadModule("MassProperties");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = MassPropertiesGui::initModule();
    MassPropertiesGui::ViewProviderMassPropertiesResult::init();

    CreateMassPropertiesCommands();

    PyMOD_Return(mod);
}
