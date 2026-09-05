// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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


#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/WidgetFactory.h>
#include <Gui/WorkbenchManipulator.h>

#include "ViewProviderSketch3D.h"
#include "WorkbenchManipulator.h"

// Command registration, defined in Command.cpp
void CreateSketcher3DCommands();


namespace Sketcher3DGui
{

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Sketcher3DGui")
    {
        initialize("Sketcher3DGui loaded.");
    }

    ~Module() override = default;
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Sketcher3DGui


/* Python entry */
PyMOD_INIT_FUNC(Sketcher3DGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    try {
        Base::Interpreter().runString("import Sketcher3D");
        Base::Interpreter().runString("import PartGui");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = Sketcher3DGui::initModule();
    Base::Console().log("Loading GUI of Sketcher3D module... done\n");

    // Type and command registration
    Sketcher3DGui::ViewProviderSketch3D::init();

    CreateSketcher3DCommands();

    // Inject Sketcher3D commands into the Sketcher workbench
    auto manip = std::make_shared<Sketcher3DGui::WorkbenchManipulator>();
    Gui::WorkbenchManipulator::installManipulator(manip);

    PyMOD_Return(mod);
}
