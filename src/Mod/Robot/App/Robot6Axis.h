/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#ifndef ROBOT_ROBOT6AXLE_H
#define ROBOT_ROBOT6AXLE_H

#include "kdl_cp/chain.hpp"
#include "kdl_cp/jntarray.hpp"

#include <Base/Persistence.h>
#include <Base/Placement.h>

namespace Robot
{

/// Definition of the Axis properties
struct AxisDefinition {
    double a;        // a of the Denavit-Hartenberg parameters (mm) 
    double alpha;    // alpha of the Denavit-Hartenberg parameters (°)
    double d;        // d of the Denavit-Hartenberg parameters (mm)
    double theta;    // a of the Denavit-Hartenberg parameters (°) 
    double rotDir;   // rotational direction (1|-1)
    double maxAngle; // soft ends + in °
    double minAngle; // soft ends - in ° 
    double velocity; // max vlocity of the axle in °/s
};


/** The representation for a 6-Axis industry grade robot
 */
class RobotExport Robot6Axis : public Base::Persistence
{
    TYPESYSTEM_HEADER();

public:
    Robot6Axis();
    ~Robot6Axis();

	// from base class
    virtual unsigned int getMemSize (void) const;
	virtual void Save (Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);

	// interface
    /// set the kinematic parameters of the robot
    void setKinematic(const AxisDefinition KinDef[6]);
    /// read the kinematic parameters of the robot from a file
    void readKinematic(const char * FileName);
    
    /// set the robot to that position, calculates the Axis
	bool setTo(const Base::Placement &To);
	bool setAxis(int Axis,double Value);
	double getAxis(int Axis);
    double getMaxAngle(int Axis);
    double getMinAngle(int Axis);
	/// calculate the new Tcp out of the Axis
	bool calcTcp(void);
	Base::Placement getTcp(void);

    //void setKinematik(const std::vector<std::vector<float> > &KinTable);


protected:
	KDL::Chain Kinematic;
	KDL::JntArray Actuall;
	KDL::JntArray Min;
	KDL::JntArray Max;
	KDL::Frame Tcp;

	double Velocity[6];
	double RotDir  [6];

};

} //namespace Part


#endif // PART_TOPOSHAPE_H
