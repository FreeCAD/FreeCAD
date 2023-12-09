/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "MBDynRevolutePinJoint.h"
#include "ASMTJoint.h"

using namespace MbD;

void MbD::MBDynRevolutePinJoint::parseMBDyn(std::string line)
{
	MBDynJoint::parseMBDyn(line);
}

void MbD::MBDynRevolutePinJoint::createASMT()
{
	MBDynJoint::createASMT();
}

std::shared_ptr<ASMTJoint> MbD::MBDynRevolutePinJoint::asmtClassNew()
{
	assert(false);
	return std::make_shared<ASMTJoint>();
}
