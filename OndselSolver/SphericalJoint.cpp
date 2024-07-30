/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "SphericalJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::SphericalJoint::SphericalJoint()
{
}

MbD::SphericalJoint::SphericalJoint(const std::string& str) : AtPointJoint(str)
{
}

void MbD::SphericalJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createAtPointConstraints();
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
