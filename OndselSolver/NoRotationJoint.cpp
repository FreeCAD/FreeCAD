/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "NoRotationJoint.h"
#include "System.h"
#include "DirectionCosineConstraintIJ.h"
#include "CREATE.h"

using namespace MbD;

MbD::NoRotationJoint::NoRotationJoint()
{
}

MbD::NoRotationJoint::NoRotationJoint(const std::string&)
{
}

void MbD::NoRotationJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 1, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
