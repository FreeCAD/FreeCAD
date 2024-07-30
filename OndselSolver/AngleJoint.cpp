/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "AngleJoint.h"
#include "CREATE.h"
#include "System.h"
#include "DirectionCosineConstraintIJ.h"

using namespace MbD;

MbD::AngleJoint::AngleJoint()
{
}

MbD::AngleJoint::AngleJoint(const std::string& str) : Joint(str)
{
}

void MbD::AngleJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto dirCosIzJz = CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 2);
		dirCosIzJz->setConstant(std::cos(theIzJz));
		addConstraint(dirCosIzJz);
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
