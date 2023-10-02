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

#include <App/DocumentObjectPy.h>
#include <Base/Placement.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "RobotObject.h"


using namespace Robot;
using namespace App;

PROPERTY_SOURCE(Robot::RobotObject, App::GeoFeature)


RobotObject::RobotObject()
{
    ADD_PROPERTY_TYPE(RobotVrmlFile,
                      (nullptr),
                      "Robot definition",
                      Prop_None,
                      "Included file with the VRML representation of the robot");
    ADD_PROPERTY_TYPE(RobotKinematicFile,
                      (nullptr),
                      "Robot definition",
                      Prop_None,
                      "Included file with kinematic definition of the robot Axis");

    ADD_PROPERTY_TYPE(Axis1,
                      (0.0),
                      "Robot kinematic",
                      Prop_None,
                      "Axis 1 angle of the robot in degre");
    ADD_PROPERTY_TYPE(Axis2,
                      (0.0),
                      "Robot kinematic",
                      Prop_None,
                      "Axis 2 angle of the robot in degre");
    ADD_PROPERTY_TYPE(Axis3,
                      (0.0),
                      "Robot kinematic",
                      Prop_None,
                      "Axis 3 angle of the robot in degre");
    ADD_PROPERTY_TYPE(Axis4,
                      (0.0),
                      "Robot kinematic",
                      Prop_None,
                      "Axis 4 angle of the robot in degre");
    ADD_PROPERTY_TYPE(Axis5,
                      (0.0),
                      "Robot kinematic",
                      Prop_None,
                      "Axis 5 angle of the robot in degre");
    ADD_PROPERTY_TYPE(Axis6,
                      (0.0),
                      "Robot kinematic",
                      Prop_None,
                      "Axis 6 angle of the robot in degre");
    ADD_PROPERTY_TYPE(Error, (""), "Robot kinematic", Prop_None, "Robot error while moving");

    ADD_PROPERTY_TYPE(Tcp, (Base::Placement()), "Robot kinematic", Prop_None, "Tcp of the robot");
    ADD_PROPERTY_TYPE(Base,
                      (Base::Placement()),
                      "Robot kinematic",
                      Prop_None,
                      "Actual base frame of the robot");
    ADD_PROPERTY_TYPE(Tool,
                      (Base::Placement()),
                      "Robot kinematic",
                      Prop_None,
                      "Tool frame of the robot (Tool)");
    ADD_PROPERTY_TYPE(ToolShape,
                      (nullptr),
                      "Robot definition",
                      Prop_None,
                      "Link to the Shape is used as Tool");
    ADD_PROPERTY_TYPE(ToolBase,
                      (Base::Placement()),
                      "Robot definition",
                      Prop_None,
                      "Defines where to connect the ToolShape");
    // ADD_PROPERTY_TYPE(Position,(Base::Placement()),"Robot definition",Prop_None,"Position of the
    // robot in the simulation");
    ADD_PROPERTY_TYPE(Home, (0), "Robot kinematic", Prop_None, "Axis position for home");
}

short RobotObject::mustExecute() const
{
    return 0;
}

PyObject* RobotObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}


void RobotObject::onChanged(const Property* prop)
{

    if (prop == &RobotKinematicFile) {
        // load the new kinematic
        robot.readKinematic(RobotKinematicFile.getValue());
    }

    if (prop == &Axis1 && !block) {
        robot.setAxis(0, Axis1.getValue());
        block = true;
        Tcp.setValue(robot.getTcp());
        block = false;
    }
    if (prop == &Axis2 && !block) {
        robot.setAxis(1, Axis2.getValue());
        block = true;
        Tcp.setValue(robot.getTcp());
        block = false;
    }
    if (prop == &Axis3 && !block) {
        robot.setAxis(2, Axis3.getValue());
        block = true;
        Tcp.setValue(robot.getTcp());
        block = false;
    }
    if (prop == &Axis4 && !block) {
        robot.setAxis(3, Axis4.getValue());
        block = true;
        Tcp.setValue(robot.getTcp());
        block = false;
    }
    if (prop == &Axis5 && !block) {
        robot.setAxis(4, Axis5.getValue());
        block = true;
        Tcp.setValue(robot.getTcp());
        block = false;
    }
    if (prop == &Axis6 && !block) {
        robot.setAxis(5, Axis6.getValue());
        block = true;
        Tcp.setValue(robot.getTcp());
        block = false;
    }
    if (prop == &Tcp && !block) {
        robot.setTo(Tcp.getValue());
        block = true;
        Axis1.setValue((float)robot.getAxis(0));
        Axis2.setValue((float)robot.getAxis(1));
        Axis3.setValue((float)robot.getAxis(2));
        Axis4.setValue((float)robot.getAxis(3));
        Axis5.setValue((float)robot.getAxis(4));
        Axis6.setValue((float)robot.getAxis(5));
        block = false;
    }
    App::GeoFeature::onChanged(prop);
}

void RobotObject::Save(Base::Writer& writer) const
{
    App::GeoFeature::Save(writer);
    robot.Save(writer);
}

void RobotObject::Restore(Base::XMLReader& reader)
{
    block = true;
    App::GeoFeature::Restore(reader);
    robot.Restore(reader);

    // set up the robot with the loaded axis position
    robot.setAxis(0, Axis1.getValue());
    robot.setAxis(1, Axis2.getValue());
    robot.setAxis(2, Axis3.getValue());
    robot.setAxis(3, Axis4.getValue());
    robot.setAxis(4, Axis5.getValue());
    robot.setAxis(5, Axis6.getValue());
    robot.setTo(Tcp.getValue());
    Tcp.setValue(robot.getTcp());
    block = false;
}
