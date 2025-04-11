/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTConstantVelocityJoint.h"
#include "ConstantVelocityJoint.h"

using namespace MbD;

std::shared_ptr<ASMTConstantVelocityJoint> MbD::ASMTConstantVelocityJoint::With()
{
	auto asmt = std::make_shared<ASMTConstantVelocityJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTConstantVelocityJoint::mbdClassNew()
{
    return CREATE<ConstantVelocityJoint>::With();
}
