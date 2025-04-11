/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "PointInLineJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::PointInLineJoint::PointInLineJoint()
{
}

MbD::PointInLineJoint::PointInLineJoint(const std::string& str) : InLineJoint(str)
{
}

void MbD::PointInLineJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createInLineConstraints();
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
