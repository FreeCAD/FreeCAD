/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTPerpendicularJoint.h"
#include "PerpendicularJoint.h"

using namespace MbD;

std::shared_ptr<ASMTPerpendicularJoint> MbD::ASMTPerpendicularJoint::With()
{
	auto asmt = std::make_shared<ASMTPerpendicularJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTPerpendicularJoint::mbdClassNew()
{
    return CREATE<PerpendicularJoint>::With();
}
