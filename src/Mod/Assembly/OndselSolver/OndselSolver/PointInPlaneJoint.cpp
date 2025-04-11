/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "PointInPlaneJoint.h"
#include "System.h"

using namespace MbD;

MbD::PointInPlaneJoint::PointInPlaneJoint()
{
}

MbD::PointInPlaneJoint::PointInPlaneJoint(const std::string&)
{
}

void MbD::PointInPlaneJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		this->createInPlaneConstraint();
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
