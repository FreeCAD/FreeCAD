/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "RevCylJoint.h"
#include "CREATE.h"
#include "DistancexyConstraintIJ.h"
#include "System.h"

using namespace MbD;

MbD::RevCylJoint::RevCylJoint()
{
}

MbD::RevCylJoint::RevCylJoint(const std::string& str) : CompoundJoint(str)
{
}

void MbD::RevCylJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto distxyIJ = DistancexyConstraintIJ::With(frmI, frmJ);
		distxyIJ->setConstant(distanceIJ);
		addConstraint(distxyIJ);
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		CompoundJoint::initializeGlobally();
	}
}
