/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ScrewJoint.h"
#include "CREATE.h"
#include "System.h"
#include "ScrewConstraintIJ.h"

using namespace MbD;

MbD::ScrewJoint::ScrewJoint()
{
}

MbD::ScrewJoint::ScrewJoint(const std::string& str) : Joint(str)
{
}
//
//void MbD::ScrewJoint::initializeLocally()
//{
//	if (!constraints->empty())
//	{
//		auto constraint = std::static_pointer_cast<ScrewConstraintIJ>(constraints->front());
//		constraint->initzIeJeIe();
//		constraint->initthezIeJe();
//	}
//	Joint::initializeLocally();
//}

void MbD::ScrewJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto screwIJ = ScrewConstraintIJ::With(frmI, frmJ);
		screwIJ->setConstant(std::numeric_limits<double>::min());
		screwIJ->pitch = pitch;
		addConstraint(screwIJ);
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}

void MbD::ScrewJoint::connectsItoJ(EndFrmsptr frmIe, EndFrmsptr frmJe)
{
	//"Subsequent prescribed motions may make frmIe, frmJe become prescribed end frames."
	//"Use newCopyEndFrameqc to prevent efrms from becoming EndFrameqct."

	frmI = frmIe->newCopyEndFrameqc();
	frmJ = frmJe->newCopyEndFrameqc();
}
