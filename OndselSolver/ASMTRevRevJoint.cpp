/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTRevRevJoint.h"
#include "RevRevJoint.h"

using namespace MbD;

std::shared_ptr<ASMTRevRevJoint> MbD::ASMTRevRevJoint::With()
{
	auto asmt = std::make_shared<ASMTRevRevJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTRevRevJoint::mbdClassNew()
{
    return CREATE<RevRevJoint>::With();
}
