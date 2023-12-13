/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "MBDynInPlaneJoint.h"
#include "ASMTPointInPlaneJoint.h"

using namespace MbD;

void MbD::MBDynInPlaneJoint::parseMBDyn(std::string line)
{
	MBDynJoint::parseMBDyn(line);
}

void MbD::MBDynInPlaneJoint::createASMT()
{
	MBDynJoint::createASMT();
}

std::shared_ptr<ASMTJoint> MbD::MBDynInPlaneJoint::asmtClassNew()
{
	return std::make_shared<ASMTPointInPlaneJoint>();
}
