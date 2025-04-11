/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "InPlaneJoint.h"
#include "CREATE.h"

MbD::InPlaneJoint::InPlaneJoint()
{
}

MbD::InPlaneJoint::InPlaneJoint(const std::string&)
{
}

void MbD::InPlaneJoint::createInPlaneConstraint()
{
	auto tranCon = CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 2);
	tranCon->setConstant(offset);
	addConstraint(tranCon);
}
