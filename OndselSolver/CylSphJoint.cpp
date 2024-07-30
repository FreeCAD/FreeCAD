/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "CylSphJoint.h"
#include "CREATE.h"
#include "DistancexyConstraintIJ.h"
#include "System.h"

using namespace MbD;

MbD::CylSphJoint::CylSphJoint()
{
}

MbD::CylSphJoint::CylSphJoint(const std::string& str) : CompoundJoint(str)
{
}

void MbD::CylSphJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto distxyIJ = DistancexyConstraintIJ::With(frmI, frmJ);
		distxyIJ->setConstant(distanceIJ);
		addConstraint(distxyIJ);
		this->root()->hasChanged = true;
	}
	else {
		CompoundJoint::initializeGlobally();
	}
}
