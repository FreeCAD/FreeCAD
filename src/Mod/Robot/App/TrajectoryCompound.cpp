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

#include "TrajectoryCompound.h"
#include "Waypoint.h"


using namespace Robot;
using namespace App;

PROPERTY_SOURCE(Robot::TrajectoryCompound, Robot::TrajectoryObject)


TrajectoryCompound::TrajectoryCompound()
{

    ADD_PROPERTY_TYPE(Source, (nullptr), "Compound", Prop_None, "list of trajectories to combine");
}

App::DocumentObjectExecReturn* TrajectoryCompound::execute()
{
    const std::vector<DocumentObject*>& Tracs = Source.getValues();
    Robot::Trajectory result;

    for (auto it : Tracs) {
        if (it->getTypeId().isDerivedFrom(Robot::TrajectoryObject::getClassTypeId())) {
            const std::vector<Waypoint*>& wps =
                static_cast<Robot::TrajectoryObject*>(it)->Trajectory.getValue().getWaypoints();
            for (auto wp : wps) {
                result.addWaypoint(*wp);
            }
        }
        else {
            return new App::DocumentObjectExecReturn(
                "Not all objects in compound are trajectories!");
        }
    }

    Trajectory.setValue(result);

    return App::DocumentObject::StdReturn;
}


// short TrajectoryCompound::mustExecute(void) const
//{
//     return 0;
// }

// void TrajectoryCompound::onChanged(const Property* prop)
//{
//
//     App::GeoFeature::onChanged(prop);
// }
