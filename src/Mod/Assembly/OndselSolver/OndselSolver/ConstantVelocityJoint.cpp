/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ConstantVelocityJoint.h"
#include "System.h"
#include "AtPointConstraintIJ.h"
#include "CREATE.h"
#include "ConstVelConstraintIJ.h"

using namespace MbD;

MbD::ConstantVelocityJoint::ConstantVelocityJoint()
{
}

MbD::ConstantVelocityJoint::ConstantVelocityJoint(const std::string& str) : AtPointJoint(str)
{
}
//
//void MbD::ConstantVelocityJoint::initializeLocally()
//{
//	if (!constraints->empty())
//	{
//		auto constraint = std::static_pointer_cast<ConstVelConstraintIJ>(constraints->back());
//		constraint->initA01IeJe();
//		constraint->initA10IeJe();
//	}
//	Joint::initializeLocally();
//}

void MbD::ConstantVelocityJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createAtPointConstraints();
		auto constVelIJ = ConstVelConstraintIJ::With(frmI, frmJ);
		constVelIJ->setConstant(0.0);
		addConstraint(constVelIJ);
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}

void MbD::ConstantVelocityJoint::connectsItoJ(EndFrmsptr frmIe, EndFrmsptr frmJe)
{
	//"Subsequent prescribed motions may make frmIe, frmJe become prescribed end frames."
	//"Use newCopyEndFrameqc to prevent efrms from becoming EndFrameqct."

	frmI = frmIe->newCopyEndFrameqc();
	frmJ = frmJe->newCopyEndFrameqc();
}
