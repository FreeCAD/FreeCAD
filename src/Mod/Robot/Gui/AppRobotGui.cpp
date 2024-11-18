/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>

#include "ViewProviderEdge2TracObject.h"
#include "ViewProviderRobotObject.h"
#include "ViewProviderTrajectory.h"
#include "ViewProviderTrajectoryCompound.h"
#include "ViewProviderTrajectoryDressUp.h"
#include "Workbench.h"


// use a different name to CreateCommand()
void CreateRobotCommands();
void CreateRobotCommandsExport();
void CreateRobotCommandsInsertRobots();
void CreateRobotCommandsTrajectory();

void loadRobotResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Robot);
    Q_INIT_RESOURCE(Robot_translation);
    Gui::Translator::instance()->refresh();
}

namespace RobotGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("RobotGui")
    {
        initialize("This module is the RobotGui module.");  // register with Python
    }

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace RobotGui


/* Python entry */
PyMOD_INIT_FUNC(RobotGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }
    try {
        Base::Interpreter().runString("import PartGui");
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import Robot");
        // set some default values
        // default speed for trajectory is 1m/s
        Base::Interpreter().runString("_DefSpeed = '1 m/s'");
        // default Cintinuity is off
        Base::Interpreter().runString("_DefCont = False");
        // default Cintinuity is off
        Base::Interpreter().runString("_DefAcceleration = '1 m/s^2'");
        // default orientation of a waypoint if no other constraint
        Base::Interpreter().runString("_DefOrientation = FreeCAD.Rotation()");
        // default displacement while e.g. picking
        Base::Interpreter().runString("_DefDisplacement = FreeCAD.Vector(0,0,0)");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }
    PyObject* mod = RobotGui::initModule();
    Base::Console().Log("Loading GUI of Robot module... done\n");

    // instantiating the commands
    CreateRobotCommands();
    CreateRobotCommandsExport();
    CreateRobotCommandsInsertRobots();
    CreateRobotCommandsTrajectory();

    // clang-format off
    // addition objects
    RobotGui::Workbench                      ::init();
    RobotGui::ViewProviderRobotObject        ::init();
    RobotGui::ViewProviderTrajectory         ::init();
    RobotGui::ViewProviderEdge2TracObject    ::init();
    RobotGui::ViewProviderTrajectoryCompound ::init();
    RobotGui::ViewProviderTrajectoryDressUp  ::init();
    // clang-format on

    // add resources and reloads the translators
    loadRobotResource();

    PyMOD_Return(mod);
}
