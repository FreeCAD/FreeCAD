/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "TranslationalJoint.h"
#include "System.h"
#include "DirectionCosineConstraintIJ.h"
#include "TranslationConstraintIJ.h"
#include "CREATE.h"

using namespace MbD;

MbD::TranslationalJoint::TranslationalJoint()
{
}

MbD::TranslationalJoint::TranslationalJoint(const std::string& str) : InLineJoint(str)
{
}

void MbD::TranslationalJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createInLineConstraints();
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 1, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
