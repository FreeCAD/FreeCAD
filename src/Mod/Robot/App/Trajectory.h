/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2009     *
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


#ifndef ROBOT_Trajectory_H
#define ROBOT_Trajectory_H

#include "kdl_cp/trajectory.hpp"

#include "Waypoint.h"

#include <Base/Persistence.h>
#include <Base/Placement.h>

#include <vector>

namespace KDL {class Trajectory_Composite;}

namespace Robot
{


/** The representation of a Trajectory
 */
class RobotExport Trajectory : public Base::Persistence
{
    TYPESYSTEM_HEADER();

public:
    Trajectory();
    Trajectory(const Trajectory&);
    ~Trajectory();

    Trajectory &operator=(const Trajectory&);

	// from base class
    virtual unsigned int getMemSize (void) const;
	virtual void Save (Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);

	// interface
    void generateTrajectory(void);
    void addWaypoint(const Waypoint &WPnt);
    unsigned int getSize(void) const{return vpcWaypoints.size();}
    const Waypoint &getWaypoint(unsigned int pos)const {return *vpcWaypoints[pos];}
    std::string getUniqueWaypointName(const char *Name) const;
    const std::vector<Waypoint*> &getWaypoints(void)const{return vpcWaypoints;}

    /// delete the last n waypoints
    void deleteLast(unsigned int n=1);
    /// return the Length (mm) of the Trajectory if -1 or of the Waypoint with the given number
    double getLength(int n=-1) const;
    /// return the duration (s) of the Trajectory if -1 or of the Waypoint with the given number
    double getDuration (int n=-1) const;
    Base::Placement getPosition(double time)const;
    double getVelocity(double time)const;


protected:

    std::vector<Waypoint*> vpcWaypoints;

    KDL::Trajectory_Composite *pcTrajectory;
};

} //namespace Part


#endif // PART_TOPOSHAPE_H
