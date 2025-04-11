/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "UniversalJoint.h"
#include "System.h"
#include "CREATE.h"

using namespace MbD;

MbD::UniversalJoint::UniversalJoint()
{
}

MbD::UniversalJoint::UniversalJoint(const std::string& str) : AtPointJoint(str)
{
}

void MbD::UniversalJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createAtPointConstraints();
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 2));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
