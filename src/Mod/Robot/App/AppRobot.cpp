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


#include "Robot6AxisPy.h"
#include "Robot6Axis.h"
#include "TrajectoryPy.h"
#include "Trajectory.h"
#include "PropertyTrajectory.h"
#include "WaypointPy.h"
#include "Waypoint.h"
#include "RobotObject.h"
#include "TrajectoryObject.h"
#include "Edge2TracObject.h"
#include "TrajectoryCompound.h"
#include "TrajectoryDressUpObject.h"

extern struct PyMethodDef Robot_methods[];

PyDoc_STRVAR(module_Robot_doc,
"This module is the Robot module.");


/* Python entry */
extern "C" {
void RobotExport initRobot()
{
    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }

    PyObject* robotModule = Py_InitModule3("Robot", Robot_methods, module_Robot_doc);   /* mod name, table ptr */
    Base::Console().Log("Loading Robot module... done\n");


    // Add Types to module
    Base::Interpreter().addType(&Robot::Robot6AxisPy          ::Type,robotModule,"Robot6Axis");
    Base::Interpreter().addType(&Robot::WaypointPy            ::Type,robotModule,"Waypoint");
    Base::Interpreter().addType(&Robot::TrajectoryPy          ::Type,robotModule,"Trajectory");


    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
 
    Robot::Robot6Axis              ::init();
    Robot::RobotObject             ::init();
    Robot::TrajectoryObject        ::init();
    Robot::Edge2TracObject         ::init();
    Robot::Waypoint                ::init();
    Robot::Trajectory              ::init();
    Robot::PropertyTrajectory      ::init();
    Robot::TrajectoryCompound      ::init();
    Robot::TrajectoryDressUpObject ::init();
}

} // extern "C"
