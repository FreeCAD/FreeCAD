/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include "ViewProviderRobotObject.h"
#include "ViewProviderTrajectory.h"
#include "ViewProviderEdge2TracObject.h"
#include "ViewProviderTrajectoryDressUp.h"
#include "ViewProviderTrajectoryCompound.h"
#include "Workbench.h"
//#include "resources/qrc_Robot.cpp"

// use a different name to CreateCommand()
void CreateRobotCommands(void);
void CreateRobotCommandsExport(void);
void CreateRobotCommandsInsertRobots(void);
void CreateRobotCommandsTrajectory(void);

void loadRobotResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Robot);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
extern struct PyMethodDef RobotGui_Import_methods[];


/* Python entry */
extern "C" {
void RobotGuiExport initRobotGui()  
{
     if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
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
        Base::Interpreter().runString("_DefAccelaration = '1 m/s^2'");
        // default orientation of a waypoint if no other constraint
        Base::Interpreter().runString("_DefOrientation = FreeCAD.Rotation()");
        // default displacement while e.g. picking
        Base::Interpreter().runString("_DefDisplacement = FreeCAD.Vector(0,0,0)");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    (void) Py_InitModule("RobotGui", RobotGui_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Robot module... done\n");

    // instantiating the commands
    CreateRobotCommands();
    CreateRobotCommandsExport();
    CreateRobotCommandsInsertRobots();
    CreateRobotCommandsTrajectory();

    // addition objects
    RobotGui::Workbench                      ::init();
	RobotGui::ViewProviderRobotObject        ::init();
	RobotGui::ViewProviderTrajectory         ::init();
	RobotGui::ViewProviderEdge2TracObject    ::init();
	RobotGui::ViewProviderTrajectoryCompound ::init();
	RobotGui::ViewProviderTrajectoryDressUp  ::init();

     // add resources and reloads the translators
    loadRobotResource();
}

} // extern "C" {
