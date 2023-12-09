/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "MBDynRevoluteHingeJoint.h"
#include "ASMTRevoluteJoint.h"

using namespace MbD;

void MbD::MBDynRevoluteHingeJoint::parseMBDyn(std::string line)
{
	MBDynJoint::parseMBDyn(line);
}

void MbD::MBDynRevoluteHingeJoint::createASMT()
{
	MBDynJoint::createASMT();
}

std::shared_ptr<ASMTJoint> MbD::MBDynRevoluteHingeJoint::asmtClassNew()
{
	return std::make_shared<ASMTRevoluteJoint>();
}
