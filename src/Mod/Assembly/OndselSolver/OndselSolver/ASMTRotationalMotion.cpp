/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTRotationalMotion.h"
#include "ASMTAssembly.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "CREATE.h"
#include "Constant.h"
#include "ASMTJoint.h"
#include "ASMTTime.h"

using namespace MbD;

std::shared_ptr<ASMTRotationalMotion> MbD::ASMTRotationalMotion::With()
{
	auto asmt = std::make_shared<ASMTRotationalMotion>();
	asmt->initialize();
	return asmt;
}

void MbD::ASMTRotationalMotion::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	if (lines[0].find("MarkerI") != std::string::npos) {
		readMarkerI(lines);
		readMarkerJ(lines);
	}
	readMotionJoint(lines);
	readRotationZ(lines);
}

void MbD::ASMTRotationalMotion::readMotionJoint(std::vector<std::string>& lines)
{
	assert(lines[0].find("MotionJoint") != std::string::npos);
	lines.erase(lines.begin());
	motionJoint = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTRotationalMotion::readRotationZ(std::vector<std::string>& lines)
{
	assert(lines[0].find("RotationZ") != std::string::npos);
	lines.erase(lines.begin());
	rotationZ = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTRotationalMotion::initMarkers()
{
	if (motionJoint == "") {
		assert(markerI != "");
		assert(markerJ != "");
	}
	else {
		auto jt = root()->jointAt(motionJoint);
		markerI = jt->markerI;
		markerJ = jt->markerJ;
	}
}

void MbD::ASMTRotationalMotion::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTMotion::createMbD(mbdSys, mbdUnits);
	auto parser = std::make_shared<SymbolicParser>();
	parser->owner = this;
	auto geoTime = owner->root()->geoTime();
	parser->variables->insert(std::make_pair("time", geoTime));
	auto userFunc = std::make_shared<BasicUserFunction>(rotationZ, 1.0);
	parser->parseUserFunction(userFunc);
	auto& geoPhi = parser->stack->top();
	//std::cout << *geoPhi << std::endl;
	geoPhi = Symbolic::times(geoPhi, sptrConstant(1.0 / mbdUnits->angle));
	geoPhi->createMbD(mbdSys, mbdUnits);
	//std::cout << *geoPhi << std::endl;
	auto simple = geoPhi->simplified(geoPhi);
	std::cout << *simple << std::endl;
	std::static_pointer_cast<ZRotation>(mbdObject)->phiBlk = simple;
}

std::shared_ptr<ItemIJ> MbD::ASMTRotationalMotion::mbdClassNew()
{
	return CREATE<ZRotation>::With();
}

void MbD::ASMTRotationalMotion::setMotionJoint(const std::string& str)
{
	motionJoint = str;
}

void MbD::ASMTRotationalMotion::setRotationZ(const std::string& rotZ)
{
	rotationZ = rotZ;
}

void MbD::ASMTRotationalMotion::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "RotationalMotion");
	ASMTItemIJ::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "MotionJoint");
	storeOnLevelString(os, level + 2, motionJoint);
	storeOnLevelString(os, level + 1, "RotationZ");
	storeOnLevelString(os, level + 2, rotationZ);
}

void MbD::ASMTRotationalMotion::storeOnTimeSeries(std::ofstream& os)
{
	os << "RotationalMotionSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
