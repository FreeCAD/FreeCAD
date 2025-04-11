/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "Units.h"

MbD::Units::Units()
{
}

MbD::Units::Units(double unitTime, double unitMass, double unitLength, double unitAngle) : time(unitTime), mass(unitMass), length(unitLength), angle(unitAngle)
{
	calc();
}

void MbD::Units::initialize()
{
}

void MbD::Units::calc()
{
	//"Calculate derived units."
	aJ = mass * length * length;
	velocity = length / time;
	omega = angle / time;
	acceleration = velocity / time;
	alpha = omega / time;
	force = mass * acceleration;
	torque = force * length;
}
