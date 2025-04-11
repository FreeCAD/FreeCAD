/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "PlanarJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::PlanarJoint::PlanarJoint()
{
}

MbD::PlanarJoint::PlanarJoint(const std::string&)
{
}

void MbD::PlanarJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		this->createInPlaneConstraint();
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
