/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "MBDynAxialRotationJoint.h"
#include "ASMTAssembly.h"
#include "ASMTRevoluteJoint.h"
#include "ASMTRotationalMotion.h"

using namespace MbD;

void MbD::MBDynAxialRotationJoint::parseMBDyn(std::string line)
{
	MBDynJoint::parseMBDyn(line);
	readFunction(arguments);
}

void MbD::MBDynAxialRotationJoint::createASMT()
{
	MBDynJoint::createASMT();
	auto asmtAsm = asmtAssembly();
	asmtMotion = std::make_shared<ASMTRotationalMotion>();
	asmtMotion->setName(name.append("Motion"));
	asmtMotion->setMotionJoint(asmtItem->fullName(""));
	asmtMotion->setRotationZ(asmtFormulaIntegral());
	asmtAsm->addMotion(asmtMotion);
	return;
}

std::shared_ptr<ASMTJoint> MbD::MBDynAxialRotationJoint::asmtClassNew()
{
	return std::make_shared<ASMTRevoluteJoint>();
}
