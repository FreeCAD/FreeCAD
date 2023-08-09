/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "SphSphJoint.h"
#include "CREATE.h"
#include "DistanceConstraintIJ.h"
#include "System.h"

using namespace MbD;

MbD::SphSphJoint::SphSphJoint()
{
}

MbD::SphSphJoint::SphSphJoint(const char* str) : CompoundJoint(str)
{
}

void MbD::SphSphJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto distxyIJ = CREATE<DistanceConstraintIJ>::With(frmI, frmJ);
		distxyIJ->setConstant(distanceIJ);
		addConstraint(distxyIJ);
		this->root()->hasChanged = true;
	}
	else {
		CompoundJoint::initializeGlobally();
	}
}
