/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <regex>

#include "MBDynTotalJoint.h"
#include "ASMTAssembly.h"
#include "ASMTJoint.h"
#include "ASMTGeneralMotion.h"

using namespace MbD;

void MbD::MBDynTotalJoint::parseMBDyn(std::string statement)
{
	MBDynJoint::parseMBDyn(statement);
	readPositionConstraints(arguments);
	readOrientationConstraints(arguments);
	return;
}

void MbD::MBDynTotalJoint::readMarkerI(std::vector<std::string>& args)
{
	mkr1 = std::make_shared<MBDynMarker>();
	mkr1->owner = this;
	mkr1->nodeStr = readStringOffTop(args);
	auto _nodeNames = nodeNames();
	std::string nodeName;
	auto it = std::find_if(args.begin(), args.end(), [&](const std::string& s) {
		auto iss = std::istringstream(s);
		iss >> nodeName;
		if (std::find(_nodeNames.begin(), _nodeNames.end(), nodeName) != _nodeNames.end()) return true;
		return false;
		});
	auto markerArgs = std::vector<std::string>(args.begin(), it);
	args.erase(args.begin(), it);
	mkr1->parseMBDynTotalJointMarker(markerArgs);
}

void MbD::MBDynTotalJoint::readMarkerJ(std::vector<std::string>& args)
{
	if (args.empty()) return;
	mkr2 = std::make_shared<MBDynMarker>();
	mkr2->owner = this;
	mkr2->nodeStr = readStringOffTop(args);
	mkr2->parseMBDynTotalJointMarker(args);
}

void MbD::MBDynTotalJoint::readPositionConstraints(std::vector<std::string>& args)
{
	std::vector<std::string> tokens{ "position", "constraint" };
	assert(lineHasTokens(popOffTop(args), tokens));
	positionConstraints = std::vector<std::string>();
	positionConstraints.push_back(readStringOffTop(args));
	positionConstraints.push_back(readStringOffTop(args));
	positionConstraints.push_back(readStringOffTop(args));
	readPositionFormulas(args);
}

void MbD::MBDynTotalJoint::readOrientationConstraints(std::vector<std::string>& args)
{
	std::vector<std::string> tokens{ "orientation", "constraint" };
	assert(lineHasTokens(popOffTop(args), tokens));
	orientationConstraints = std::vector<std::string>();
	orientationConstraints.push_back(readStringOffTop(args));
	orientationConstraints.push_back(readStringOffTop(args));
	orientationConstraints.push_back(readStringOffTop(args));
	readOrientationFormulas(args);
}

void MbD::MBDynTotalJoint::readPositionFormulas(std::vector<std::string>& args)
{
	std::string str = readStringOffTop(args);
	if (str == "null") return;
	assert(false);
}

void MbD::MBDynTotalJoint::readOrientationFormulas(std::vector<std::string>& args)
{
	std::string str = readStringOffTop(args);
	if (str == "null") { return; }
	else if (str == "single") {
		auto vec3 = readVector3(args);
		assert(vec3->at(0) == 0 && vec3->at(1) == 0 && vec3->at(2) == 1);
		assert(readStringOffTop(args) == "string");
		formula = popOffTop(args);
		formula = std::regex_replace(formula, std::regex("\""), "");
		orientationFormulas = std::vector<std::string>();
		for (auto& status : orientationConstraints) {
			if (status == "active") {
				orientationFormulas.push_back("");
			}
			else if (status == "rotation") {
				orientationFormulas.push_back(formula);
			}
			else { assert(false); }
		}
		return;
	}
	assert(false);
}

void MbD::MBDynTotalJoint::createASMT()
{
	mkr1->createASMT();
	if (mkr2) mkr2->createASMT();
	auto asmtAsm = asmtAssembly();
	auto asmtMotion = std::make_shared<ASMTGeneralMotion>();
	asmtItem = asmtMotion;
	asmtMotion->setName(name);
	asmtMotion->setMarkerI(mkr1->asmtItem->fullName(""));
	asmtMotion->setMarkerJ(mkr2->asmtItem->fullName(""));
	asmtAsm->addMotion(asmtMotion);
	for (int i = 0; i < 3; i++)
	{
		asmtMotion->rIJI->atiput(i, asmtFormula(positionFormulas.at(i)));
		asmtMotion->angIJJ->atiput(i, asmtFormula(orientationFormulas.at(i)));
	}
}
