/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <regex>

#include "MBDynJoint.h"
#include "ASMTMarker.h"
#include "ASMTPart.h"
#include "ASMTJoint.h"
#include "ASMTAssembly.h"
#include "ASMTRevoluteJoint.h"
#include "ASMTRotationalMotion.h"
#include "ASMTPointInLineJoint.h"
#include "ASMTNoRotationJoint.h"
#include "ASMTFixedJoint.h"
#include "ASMTSphericalJoint.h"
#include "MBDynTotalJoint.h"
#include "MBDynClampJoint.h"
#include "MBDynAxialRotationJoint.h"
#include "MBDynDriveHingeJoint.h"
#include "MBDynInLineJoint.h"
#include "MBDynInPlaneJoint.h"
#include "MBDynPrismaticJoint.h"
#include "MBDynRevoluteHingeJoint.h"
#include "MBDynRevolutePinJoint.h"
#include "MBDynSphericalHingeJoint.h"

using namespace MbD;

std::shared_ptr<MBDynJoint> MbD::MBDynJoint::newJoint(std::string statement)
{
	//std::shared_ptr<MBDynJoint> joint;
	//std::vector<std::string> tokens{ "total", "joint" };
	std::vector<std::string> tokens;
	tokens = { "axial", "rotation" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynAxialRotationJoint>();
	}
	tokens = { "clamp" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynClampJoint>();
	}
	tokens = { "drive", "hinge" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynDriveHingeJoint>();
	}
	tokens = { "in", "line" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynInLineJoint>();
	}
	tokens = { "in", "plane" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynInPlaneJoint>();
	}
	tokens = { "prismatic" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynPrismaticJoint>();
	}
	tokens = { "revolute", "hinge" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynRevoluteHingeJoint>();
	}
	tokens = { "revolute", "pin" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynRevolutePinJoint>();
	}
	tokens = { "spherical", "hinge" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynSphericalHingeJoint>();
	}
	tokens = { "total", "joint" };
	if (lineHasTokens(statement, tokens)) {
		return std::make_shared<MBDynTotalJoint>();
	}
	return std::make_shared<MBDynJoint>();
}

void MbD::MBDynJoint::initialize()
{
}

void MbD::MBDynJoint::parseMBDyn(std::string line)
{
	jointString = line;
	arguments = collectArgumentsFor("joint", line);
	readName(arguments);
	readJointType(arguments);
	readMarkerI(arguments);
	readMarkerJ(arguments);

}

void MbD::MBDynJoint::readJointType(std::vector<std::string>& args)
{
	joint_type = readJointTypeOffTop(args);
}

//void MbD::MBDynJoint::parseMBDyn0(std::string line)
//{
//	jointString = line;
//	arguments = collectArgumentsFor("joint", line);
//	auto ss = std::stringstream();
//	name = readStringOffTop(arguments);
//	auto iss = std::istringstream(arguments.at(0));
//	iss >> joint_type;
//	if (joint_type == "axial") {
//		ss << joint_type;
//		iss >> joint_type;
//		if (joint_type == "rotation") {
//			ss << " " << joint_type;
//			joint_type = ss.str();
//		}
//		else {
//			assert(false);
//		}
//		arguments.erase(arguments.begin());
//		readMarkerI(arguments);
//		readMarkerJ(arguments);
//		readFunction(arguments);
//		return;
//	}
//	else if (joint_type == "revolute") {
//		ss << joint_type;
//		iss >> joint_type;
//		if (joint_type == "hinge") {
//			ss << " " << joint_type;
//			joint_type = ss.str();
//		}
//		else {
//			assert(false);
//		}
//	}
//	else if (joint_type == "spherical") {
//		ss << joint_type;
//		iss >> joint_type;
//		if (joint_type == "hinge") {
//			ss << " " << joint_type;
//			joint_type = ss.str();
//		}
//		else {
//			assert(false);
//		}
//	}
//	else if (joint_type == "drive") {
//		ss << joint_type;
//		iss >> joint_type;
//		if (joint_type == "hinge") {
//			ss << " " << joint_type;
//			joint_type = ss.str();
//		}
//		else {
//			assert(false);
//		}
//		arguments.erase(arguments.begin());
//		readMarkerI(arguments);
//		readMarkerJ(arguments);
//		readFunction(arguments);
//		return;
//	}
//	else if (joint_type == "in") {
//		ss << joint_type;
//		iss >> joint_type;
//		if (joint_type == "line") {
//			ss << " " << joint_type;
//			joint_type = ss.str();
//		}
//		else {
//			assert(false);
//		}
//	}
//	else if (joint_type == "total") {
//		ss << joint_type;
//		iss >> joint_type;
//		if (joint_type == "joint") {
//			ss << " " << joint_type;
//			joint_type = ss.str();
//		}
//		else {
//			assert(false);
//		}
//		arguments.erase(arguments.begin());
//		readTotalJointMarkerI(arguments);
//		readTotalJointMarkerJ(arguments);
//		readTotalJointFunction(arguments);
//		return;
//	}
//	else if (joint_type == "clamp") {
//		//mkr1 should be on assembly which doesn't exist in MBDyn
//		//mkr2 is on the node
//		arguments.erase(arguments.begin());
//		mkr1 = std::make_shared<MBDynMarker>();
//		mkr1->owner = this;
//		mkr1->nodeStr = "Assembly";
//		mkr1->rPmP = std::make_shared<FullColumn<double>>(3);
//		mkr1->aAPm = FullMatrix<double>::identitysptr(3);
//		readClampMarkerJ(arguments);
//		return;
//	}
//	else if (joint_type == "prismatic") {
//		noop();
//	}
//	else {
//		assert(false);
//	}
//	arguments.erase(arguments.begin());
//	readMarkerI(arguments);
//	readMarkerJ(arguments);
//}

void MbD::MBDynJoint::readMarkerI(std::vector<std::string>& args)
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
	mkr1->parseMBDyn(markerArgs);
}

void MbD::MBDynJoint::readMarkerJ(std::vector<std::string>& args)
{
	if (args.empty()) return;
	mkr2 = std::make_shared<MBDynMarker>();
	mkr2->owner = this;
	mkr2->nodeStr = readStringOffTop(args);
	mkr2->parseMBDyn(args);
}

void MbD::MBDynJoint::readTotalJointMarkerI(std::vector<std::string>& args)
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

void MbD::MBDynJoint::readTotalJointMarkerJ(std::vector<std::string>& args)
{
	if (args.empty()) return;
	mkr2 = std::make_shared<MBDynMarker>();
	mkr2->owner = this;
	mkr2->nodeStr = readStringOffTop(args);
	mkr2->parseMBDynTotalJointMarker(args);
}

void MbD::MBDynJoint::readClampMarkerJ(std::vector<std::string>& args)
{
	if (args.empty()) return;
	mkr2 = std::make_shared<MBDynMarker>();
	mkr2->owner = this;
	mkr2->nodeStr = readStringOffTop(args);
	mkr2->parseMBDynClamp(args);
}

void MbD::MBDynJoint::readFunction(std::vector<std::string>& args)
{
	if (args.empty()) return;
	std::string str;
	auto iss = std::istringstream(args.at(0));
	iss >> str;
	if (str.find("ramp") != std::string::npos) {
		args.erase(args.begin());
		std::string slope, initValue, initTime, finalTime;
		slope = popOffTop(args);
		initTime = popOffTop(args);
		finalTime = popOffTop(args);
		initValue = popOffTop(args);
		//f = slope*(time - t0) + f0
		auto ss = std::stringstream();
		ss << slope << "*(time - " << initTime << ") + " << initValue;
		formula = ss.str();
	}
	else if (str.find("single") != std::string::npos) {
		args.erase(args.begin());
		auto vec3 = readVector3(args);
		assert(vec3->at(0) == 0 && vec3->at(1) == 0 && vec3->at(2) == 1);
		assert(readStringOffTop(args) == "string");
		formula = popOffTop(args);
		formula = std::regex_replace(formula, std::regex("\""), "");
	}
	else if (str.find("string") != std::string::npos) {
		args.erase(args.begin());
		formula = popOffTop(args);
		formula = std::regex_replace(formula, std::regex("\""), "");
	}
	else {
		assert(false);
	}
}

void MbD::MBDynJoint::readTotalJointFunction(std::vector<std::string>& args)
{
	std::vector<std::string> tokens{ "position", "constraint" };
	assert(lineHasTokens(args[0], tokens));
	args.erase(args.begin());
	assert(readStringOffTop(args) == "active");
	assert(readStringOffTop(args) == "active");
	assert(readStringOffTop(args) == "active");
	assert(readStringOffTop(args) == "null");
	std::vector<std::string> tokens1{ "orientation", "constraint" };
	assert(lineHasTokens(args[0], tokens1));
	args.erase(args.begin());
	assert(readStringOffTop(args) == "active");
	assert(readStringOffTop(args) == "active");
	assert(readStringOffTop(args) == "rotation");
	readFunction(args);
}

void MbD::MBDynJoint::createASMT()
{
	mkr1->createASMT();
	if (mkr2) mkr2->createASMT();
	auto asmtAsm = asmtAssembly();
	auto asmtJoint = asmtClassNew();
	asmtItem = asmtJoint;
	asmtJoint->setName(name);
	asmtJoint->setMarkerI(mkr1->asmtItem->fullName(""));
	asmtJoint->setMarkerJ(mkr2->asmtItem->fullName(""));
	asmtAsm->addJoint(asmtJoint);
}

std::shared_ptr<ASMTJoint> MbD::MBDynJoint::asmtClassNew()
{
	assert(false);
	return std::make_shared<ASMTJoint>();
}

//void MbD::MBDynJoint::createASMT()
//{
//	mkr1->createASMT();
//	if (mkr2) mkr2->createASMT();
//	std::shared_ptr<ASMTJoint> asmtJoint;
//	if (joint_type == "clamp") {
//		auto asmtAsm = asmtAssembly();
//		asmtJoint = std::make_shared<ASMTFixedJoint>();
//		asmtJoint->setName(name);
//		asmtJoint->setMarkerI(mkr1->asmtItem->fullName(""));
//		asmtJoint->setMarkerJ(mkr2->asmtItem->fullName(""));
//		asmtAsm->addJoint(asmtJoint);
//		return;
//	}
//	if (joint_type == "axial rotation") {
//		auto asmtAsm = asmtAssembly();
//		asmtJoint = std::make_shared<ASMTRevoluteJoint>();
//		asmtItem = asmtJoint;
//		asmtJoint->setName(name);
//		asmtJoint->setMarkerI(mkr1->asmtItem->fullName(""));
//		asmtJoint->setMarkerJ(mkr2->asmtItem->fullName(""));
//		asmtAsm->addJoint(asmtJoint);
//		auto asmtMotion = std::make_shared<ASMTRotationalMotion>();
//		asmtItem = asmtMotion;
//		asmtMotion->setName(name.append("Motion"));
//		asmtMotion->setMotionJoint(asmtJoint->fullName(""));
//		asmtMotion->setRotationZ(asmtFormulaIntegral());
//		asmtAsm->addMotion(asmtMotion);
//		return;
//	}
//	if (joint_type == "drive hinge") {
//		auto asmtAsm = asmtAssembly();
//		auto asmtMotion = std::make_shared<ASMTRotationalMotion>();
//		asmtItem = asmtMotion;
//		asmtMotion->setName(name);
//		asmtMotion->setMarkerI(mkr1->asmtItem->fullName(""));
//		asmtMotion->setMarkerJ(mkr2->asmtItem->fullName(""));
//		asmtMotion->setRotationZ(formula);
//		asmtAsm->addMotion(asmtMotion);
//		return;
//	}
//	if (joint_type == "revolute hinge") {
//		asmtJoint = std::make_shared<ASMTRevoluteJoint>();
//	}
//	else if (joint_type == "spherical hinge") {
//		asmtJoint = std::make_shared<ASMTSphericalJoint>();
//	}
//	else if (joint_type == "in line") {
//		asmtJoint = std::make_shared<ASMTPointInLineJoint>();
//	}
//	else if (joint_type == "prismatic") {
//		asmtJoint = std::make_shared<ASMTNoRotationJoint>();
//	}
//	else {
//		assert(false);
//	}
//	asmtItem = asmtJoint;
//	asmtJoint->setName(name);
//	asmtJoint->setMarkerI(mkr1->asmtItem->fullName(""));
//	asmtJoint->setMarkerJ(mkr2->asmtItem->fullName(""));
//	asmtAssembly()->addJoint(asmtJoint);
//}

std::string MbD::MBDynJoint::asmtFormula()
{
	auto ss = std::stringstream();
	std::string drivestr = "model::drive";
	size_t previousPos = 0;
	auto pos = formula.find(drivestr);
	ss << formula.substr(previousPos, pos - previousPos);
	while (pos != std::string::npos) {
		previousPos = pos;
		pos = formula.find('(', pos + 1);
		previousPos = pos;
		pos = formula.find(',', pos + 1);
		auto driveName = formula.substr(previousPos + 1, pos - previousPos - 1);
		driveName = readToken(driveName);
		previousPos = pos;
		pos = formula.find(')', pos + 1);
		auto varName = formula.substr(previousPos + 1, pos - previousPos - 1);
		varName = readToken(varName);
		//Insert drive formula
		ss << formulaFromDrive(driveName, varName);
		previousPos = pos;
		pos = formula.find(drivestr, pos + 1);
		ss << formula.substr(previousPos + 1, pos - previousPos);
	}
	return ss.str();
}

std::string MbD::MBDynJoint::asmtFormula(std::string mbdynFormula)
{
	auto ss = std::stringstream();
	std::string drivestr = "model::drive";
	size_t previousPos = 0;
	auto pos = mbdynFormula.find(drivestr);
	ss << mbdynFormula.substr(previousPos, pos - previousPos);
	while (pos != std::string::npos) {
		previousPos = pos;
		pos = mbdynFormula.find('(', pos + 1);
		previousPos = pos;
		pos = mbdynFormula.find(',', pos + 1);
		auto driveName = mbdynFormula.substr(previousPos + 1, pos - previousPos - 1);
		driveName = readToken(driveName);
		previousPos = pos;
		pos = mbdynFormula.find(')', pos + 1);
		auto varName = mbdynFormula.substr(previousPos + 1, pos - previousPos - 1);
		varName = readToken(varName);
		//Insert drive mbdynFormula
		ss << formulaFromDrive(driveName, varName);
		previousPos = pos;
		pos = mbdynFormula.find(drivestr, pos + 1);
		ss << mbdynFormula.substr(previousPos + 1, pos - previousPos);
	}
	return ss.str();
}

std::string MbD::MBDynJoint::asmtFormulaIntegral()
{
	auto ss = std::stringstream();
	ss << "integral(time, " << asmtFormula() << ")";
	return ss.str();
}
