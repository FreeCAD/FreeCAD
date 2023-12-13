/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "MBDynPrismaticJoint.h"
#include "ASMTNoRotationJoint.h"

using namespace MbD;

void MbD::MBDynPrismaticJoint::parseMBDyn(std::string line)
{
	MBDynJoint::parseMBDyn(line);
}

void MbD::MBDynPrismaticJoint::createASMT()
{
	MBDynJoint::createASMT();
}

std::shared_ptr<ASMTJoint> MbD::MBDynPrismaticJoint::asmtClassNew()
{
	return std::make_shared<ASMTNoRotationJoint>();
}
