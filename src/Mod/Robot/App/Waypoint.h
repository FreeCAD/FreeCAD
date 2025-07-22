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

#ifndef ROBOT_WAYPOINT_H
#define ROBOT_WAYPOINT_H

#include <Base/Persistence.h>
#include <Base/Placement.h>
#include <Mod/Robot/RobotGlobal.h>


namespace Robot
{

/** The representation of a waypoint in a trajectory
 */
class RobotExport Waypoint: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum WaypointType
    {
        UNDEF,
        PTP,
        LINE,
        CIRC,
        WAIT
    };

    Waypoint();
    /// full constructor
    Waypoint(const char* name,
             const Base::Placement& endPos,
             WaypointType type = Waypoint::LINE,
             float velocity = 2000.0,
             float acceleration = 100.0,
             bool cont = false,
             unsigned int tool = 0,
             unsigned int base = 0);

    ~Waypoint() override;

    // from base class
    unsigned int getMemSize() const override;
    void Save(Base::Writer& /*writer*/) const override;
    void Restore(Base::XMLReader& /*reader*/) override;


    std::string Name;
    WaypointType Type;
    float Velocity;
    float Acceleration;
    bool Cont;
    unsigned int Tool, Base;
    Base::Placement EndPos;
};

}  // namespace Robot


#endif  // ROBOT_WAYPOINT_H
