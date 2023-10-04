/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "RackPinJoint.h"
#include "CREATE.h"
#include "System.h"
#include "RackPinConstraintIJ.h"

using namespace MbD;

MbD::RackPinJoint::RackPinJoint()
{
}

MbD::RackPinJoint::RackPinJoint(const char* str) : Joint(str)
{
}

void MbD::RackPinJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto rackPinIJ = CREATE<RackPinConstraintIJ>::With(frmI, frmJ);
		rackPinIJ->setConstant(aConstant);
		rackPinIJ->pitchRadius = pitchRadius;
		addConstraint(rackPinIJ);
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}

void MbD::RackPinJoint::connectsItoJ(EndFrmsptr frmIe, EndFrmsptr frmJe)
{
	//"OODS J is on pinion. z axis is axis of pinion."
	//"OODS I is on rack. x axis is axis of rack. z axis is parallel to axis of pinion."
	//"Subsequent prescribed motions may make frmIe, frmJe become prescribed end frames."
	//"Use newCopyEndFrameqc to prevent efrms from becoming EndFrameqct."

	frmI = frmIe->newCopyEndFrameqc();
	frmJ = frmJe->newCopyEndFrameqc();
}
