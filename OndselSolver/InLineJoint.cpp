/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "InLineJoint.h"
#include "CREATE.h"

MbD::InLineJoint::InLineJoint()
{
}

MbD::InLineJoint::InLineJoint(const std::string&)
{
}

void MbD::InLineJoint::createInLineConstraints()
{
	addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 0));
	addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 1));
}
