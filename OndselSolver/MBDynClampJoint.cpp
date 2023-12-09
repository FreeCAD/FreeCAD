/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "MBDynClampJoint.h"
#include "ASMTAssembly.h"
#include "ASMTFixedJoint.h"

using namespace MbD;

void MbD::MBDynClampJoint::parseMBDyn(std::string statement)
{
	MBDynJoint::parseMBDyn(statement);
	assert(joint_type == "clamp");
	return;
}

void MbD::MBDynClampJoint::createASMT()
{
	MBDynJoint::createASMT();
}

void MbD::MBDynClampJoint::readMarkerI(std::vector<std::string>& args)
{
	//mkr1 should be on assembly which doesn't exist in MBDyn
	//mkr2 is on the node
	mkr1 = std::make_shared<MBDynMarker>();
	mkr1->owner = this;
	mkr1->nodeStr = "Assembly";
	mkr1->rPmP = std::make_shared<FullColumn<double>>(3);
	mkr1->aAPm = FullMatrix<double>::identitysptr(3);
}

void MbD::MBDynClampJoint::readMarkerJ(std::vector<std::string>& args)
{
	if (args.empty()) return;
	mkr2 = std::make_shared<MBDynMarker>();
	mkr2->owner = this;
	mkr2->nodeStr = readStringOffTop(args);
	mkr2->parseMBDynClamp(args);
}

std::shared_ptr<ASMTJoint> MbD::MBDynClampJoint::asmtClassNew()
{
	return std::make_shared<ASMTFixedJoint>();
}
