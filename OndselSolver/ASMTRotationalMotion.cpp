/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "ASMTRotationalMotion.h"
#include "ASMTAssembly.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "CREATE.h"
#include "Constant.h"
#include "ASMTJoint.h"
#include "ASMTTime.h"

using namespace MbD;

void MbD::ASMTRotationalMotion::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
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
	auto geoPhi = parser->stack->top();
	geoPhi = Symbolic::times(geoPhi, std::make_shared<Constant>(1.0 / mbdUnits->angle));
	geoPhi->createMbD(mbdSys, mbdUnits);
	std::static_pointer_cast<ZRotation>(mbdObject)->phiBlk = geoPhi->simplified(geoPhi);
}

std::shared_ptr<Joint> MbD::ASMTRotationalMotion::mbdClassNew()
{
	return CREATE<ZRotation>::With();
}

void MbD::ASMTRotationalMotion::setMotionJoint(std::string str)
{
	motionJoint = str;
}

void MbD::ASMTRotationalMotion::setRotationZ(std::string rotZ)
{
	rotationZ = rotZ;
}
