/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "Simulation.h"


using namespace Robot;
using namespace std;
using namespace KDL;

//===========================================================================
// Simulation class
//===========================================================================

Simulation::Simulation(const Robot::Trajectory& Trac, Robot::Robot6Axis& Rob)
    : Trac(Trac)
    , Rob(Rob)
{
    // simulate a trajectory with only one waypoint make no sense!
    assert(Trac.getSize() > 1);


    startAxis[0] = Rob.getAxis(0);
    startAxis[1] = Rob.getAxis(1);
    startAxis[2] = Rob.getAxis(2);
    startAxis[3] = Rob.getAxis(3);
    startAxis[4] = Rob.getAxis(4);
    startAxis[5] = Rob.getAxis(5);

    setToTime(0);
}

Simulation::~Simulation() = default;

void Simulation::step(double tick)
{
    Pos += tick;
}

void Simulation::setToWaypoint(unsigned int)
{}

void Simulation::setToTime(float t)
{
    Pos = t;
    Base::Placement NeededPos = Trac.getPosition(Pos);
    NeededPos = NeededPos * Tool.inverse();
    Rob.setTo(NeededPos);
    Axis[0] = Rob.getAxis(0);
    Axis[1] = Rob.getAxis(1);
    Axis[2] = Rob.getAxis(2);
    Axis[3] = Rob.getAxis(3);
    Axis[4] = Rob.getAxis(4);
    Axis[5] = Rob.getAxis(5);
}

void Simulation::reset()
{
    Rob.setAxis(0, startAxis[0]);
    Rob.setAxis(1, startAxis[1]);
    Rob.setAxis(2, startAxis[2]);
    Rob.setAxis(3, startAxis[3]);
    Rob.setAxis(4, startAxis[4]);
    Rob.setAxis(5, startAxis[5]);

    Base::Placement NeededPos = Trac.getPosition(0.0);
    NeededPos = NeededPos * Tool.inverse();
    Rob.setTo(NeededPos);

    Axis[0] = Rob.getAxis(0);
    Axis[1] = Rob.getAxis(1);
    Axis[2] = Rob.getAxis(2);
    Axis[3] = Rob.getAxis(3);
    Axis[4] = Rob.getAxis(4);
    Axis[5] = Rob.getAxis(5);
}
