/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "TrajectoryDressUpObject.h"
#include "Waypoint.h"


using namespace Robot;
using namespace App;

PROPERTY_SOURCE(Robot::TrajectoryDressUpObject, Robot::TrajectoryObject)

const char* TrajectoryDressUpObject::ContTypeEnums[] = {"DontChange",
                                                        "Continues",
                                                        "Discontinues",
                                                        nullptr};
const char* TrajectoryDressUpObject::AddTypeEnums[] = {"DontChange",
                                                       "UseOrientation",
                                                       "AddPosition",
                                                       "AddOrintation",
                                                       "AddPositionAndOrientation",
                                                       nullptr};

TrajectoryDressUpObject::TrajectoryDressUpObject()
{

    ADD_PROPERTY_TYPE(Source, (nullptr), "TrajectoryDressUp", Prop_None, "Trajectory to dress up");
    ADD_PROPERTY_TYPE(Speed, (1000), "TrajectoryDressUp", Prop_None, "Speed to use");
    ADD_PROPERTY_TYPE(UseSpeed, (0), "TrajectoryDressUp", Prop_None, "Switch the speed usage on");
    ADD_PROPERTY_TYPE(Acceleration, (1000), "TrajectoryDressUp", Prop_None, "Acceleration to use");
    ADD_PROPERTY_TYPE(UseAcceleration,
                      (0),
                      "TrajectoryDressUp",
                      Prop_None,
                      "Switch the acceleration usage on");
    ADD_PROPERTY_TYPE(ContType,
                      ((long)0),
                      "TrajectoryDressUp",
                      Prop_None,
                      "Define the dress up of continuity");
    ContType.setEnums(ContTypeEnums);
    ADD_PROPERTY_TYPE(PosAdd,
                      (Base::Placement()),
                      "TrajectoryDressUp",
                      Prop_None,
                      "Position & Orientation to use");
    ADD_PROPERTY_TYPE(AddType,
                      ((long)0),
                      "TrajectoryDressUp",
                      Prop_None,
                      "How to change the Position & Orientation");
    AddType.setEnums(AddTypeEnums);
}

App::DocumentObjectExecReturn* TrajectoryDressUpObject::execute()
{
    Robot::Trajectory result;

    App::DocumentObject* link = Source.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No object linked");
    }
    if (!link->getTypeId().isDerivedFrom(Robot::TrajectoryObject::getClassTypeId())) {
        return new App::DocumentObjectExecReturn("Linked object is not a Trajectory object");
    }

    const std::vector<Waypoint*>& wps =
        static_cast<Robot::TrajectoryObject*>(link)->Trajectory.getValue().getWaypoints();
    for (auto wp : wps) {
        Waypoint wpt = *wp;
        if (UseSpeed.getValue()) {
            wpt.Velocity = Speed.getValue();
        }
        if (UseAcceleration.getValue()) {
            wpt.Acceleration = Acceleration.getValue();
        }
        switch (ContType.getValue()) {
            case 0:
                break;
            case 1:
                wpt.Cont = true;
                break;
            case 2:
                wpt.Cont = false;
                break;
            default:
                assert(0);  // must not happen!
        }
        switch (AddType.getValue()) {
                // do nothing
            case 0:
                break;
                // use orientation
            case 1:
                wpt.EndPos.setRotation(PosAdd.getValue().getRotation());
                break;
                // add position
            case 2:
                wpt.EndPos.setPosition(wpt.EndPos.getPosition() + PosAdd.getValue().getPosition());
                break;
                // add orientation
            case 3:
                wpt.EndPos.setRotation(wpt.EndPos.getRotation() * PosAdd.getValue().getRotation());
                break;
                // add orientation & position
            case 4:
                wpt.EndPos = wpt.EndPos * PosAdd.getValue();
                break;
            default:
                assert(0);  // must not happen!
        }

        result.addWaypoint(wpt);
    }

    // set the resulting Trajectory to the object
    Trajectory.setValue(result);

    return App::DocumentObject::StdReturn;
}

void TrajectoryDressUpObject::onChanged(const Property* prop)
{
    App::GeoFeature::onChanged(prop);
}
