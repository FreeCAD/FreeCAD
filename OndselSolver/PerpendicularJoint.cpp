/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "PerpendicularJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::PerpendicularJoint::PerpendicularJoint()
{
}

MbD::PerpendicularJoint::PerpendicularJoint(const std::string&)
{
}

void MbD::PerpendicularJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 2));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
