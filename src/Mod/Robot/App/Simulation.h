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

 


#ifndef _Simulation_h_
#define _Simulation_h_

#include <Base/Vector3D.h>
#include <Base/Placement.h>
#include <string>

#include "Trajectory.h"
#include "Robot6Axis.h"

namespace Robot
{

/** Algo class for projecting shapes and creating SVG output of it
 */
class RobotExport Simulation
{

public:
	/// Constructor
	Simulation(const Trajectory &Trac,Robot6Axis &Rob);
	virtual ~Simulation();

	double getLength(void){return Trac.getLength();}
	double getDuration(void){return Trac.getDuration();}

    Base::Placement getPosition(void){return Trac.getPosition(Pos);}
    double getVelocity(void){return Trac.getVelocity(Pos);}

	void step(double tick);
    void setToWaypoint(unsigned int n);
    void setToTime(float t);
    // apply the start axis angles and set to time 0. Restors the exact start position
    void reset(void);

	double Pos;
	double Axis[6];
	double startAxis[6];

    Trajectory Trac;
    Robot6Axis &Rob;
    Base::Placement Tool;
};



} //namespace Robot



#endif
