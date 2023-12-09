/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "MBDynInLineJoint.h"
#include "ASMTPointInLineJoint.h"

using namespace MbD;

void MbD::MBDynInLineJoint::parseMBDyn(std::string line)
{
	MBDynJoint::parseMBDyn(line);
}

void MbD::MBDynInLineJoint::createASMT()
{
	MBDynJoint::createASMT();
}

std::shared_ptr<ASMTJoint> MbD::MBDynInLineJoint::asmtClassNew()
{
	return std::make_shared<ASMTPointInLineJoint>();
}
