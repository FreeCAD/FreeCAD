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

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Language/Translator.h>

#include <QString>

#include <3rdParty/GSL/include/gsl/pointers>

#include "Manipulator.h"

void loadStartResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Start);
    Q_INIT_RESOURCE(Start_translation);
    Gui::Translator::instance()->refresh();
}

namespace StartGui
{
extern PyObject* initModule();
}


namespace StartGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("StartGui")
    {
        initialize("This module is the StartGui module.");  // register with Python
    }
};

PyObject* initModule()
{
    auto newModule = gsl::owner<Module*>(new Module);
    return Base::Interpreter().addModule(newModule);  // Transfer ownership
}

}  // namespace StartGui

/* Python entry */
PyMOD_INIT_FUNC(StartGui)
{
    Base::Console().Log("Loading GUI of Start module... ");
    PyObject* mod = StartGui::initModule();
    auto manipulator = std::make_shared<StartGui::Manipulator>();
    Gui::WorkbenchManipulator::installManipulator(manipulator);
    loadStartResource();
    Base::Console().Log("done\n");

    PyMOD_Return(mod);
}
