/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "RevRevJoint.h"
#include "CREATE.h"
#include "DistancexyConstraintIJ.h"
#include "System.h"

using namespace MbD;

MbD::RevRevJoint::RevRevJoint()
{
}

MbD::RevRevJoint::RevRevJoint(const std::string& str) : CompoundJoint(str)
{
}

void MbD::RevRevJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto distxyIJ = DistancexyConstraintIJ::With(frmI, frmJ);
		distxyIJ->setConstant(distanceIJ);
		addConstraint(distxyIJ);
		addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 2));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		CompoundJoint::initializeGlobally();
	}
}
