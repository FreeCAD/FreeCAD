/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "AtPointJoint.h"
#include "System.h"
#include "CREATE.h"

using namespace MbD;

MbD::AtPointJoint::AtPointJoint()
{
}

MbD::AtPointJoint::AtPointJoint(const std::string& str) : Joint(str)
{
}

void MbD::AtPointJoint::createAtPointConstraints()
{
	addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 0));
	addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 1));
	addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 2));
}
