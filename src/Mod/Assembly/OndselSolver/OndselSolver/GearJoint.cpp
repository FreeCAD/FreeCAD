/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "GearJoint.h"
#include "CREATE.h"
#include "GearConstraintIJ.h"
#include "System.h"

using namespace MbD;

MbD::GearJoint::GearJoint()
{
}

MbD::GearJoint::GearJoint(const std::string&)
{
}
//
//void MbD::GearJoint::initializeLocally()
//{
//	if (!constraints->empty())
//	{
//		auto constraint = std::static_pointer_cast<GearConstraintIJ>(constraints->back());
//		constraint->initorbitsIJ();
//	}
//	Joint::initializeLocally();
//}

void MbD::GearJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto gearIJ = GearConstraintIJ::With(frmI, frmJ);
		gearIJ->radiusI = radiusI;
		gearIJ->radiusJ = radiusJ;
		gearIJ->setConstant(std::numeric_limits<double>::min());
		addConstraint(gearIJ);
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
