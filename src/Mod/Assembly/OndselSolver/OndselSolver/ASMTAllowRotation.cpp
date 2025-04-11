/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "ASMTAllowRotation.h"
#include "ASMTAssembly.h"
#include "ASMTJoint.h"
#include "AllowZRotation.h"

using namespace MbD;

std::shared_ptr<ASMTAllowRotation> MbD::ASMTAllowRotation::With()
{
	auto asmtAllowRotation = std::make_shared<ASMTAllowRotation>();
	return asmtAllowRotation;
}

void MbD::ASMTAllowRotation::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	if (lines[0].find("MarkerI") != std::string::npos) {
		readMarkerI(lines);
		readMarkerJ(lines);
	}
	readMotionJoint(lines);
}

void MbD::ASMTAllowRotation::readMotionJoint(std::vector<std::string>& lines)
{
	assert(lines[0].find("MotionJoint") != std::string::npos);
	lines.erase(lines.begin());
	motionJoint = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTAllowRotation::initMarkers()
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

std::shared_ptr<ItemIJ> MbD::ASMTAllowRotation::mbdClassNew()
{
	return AllowZRotation::With();
}

void MbD::ASMTAllowRotation::setMotionJoint(const std::string& motionJoint)
{
    (void) motionJoint;
}

void MbD::ASMTAllowRotation::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "AllowRotation");
	ASMTItemIJ::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "MotionJoint");
	storeOnLevelString(os, level + 2, motionJoint);
}

void MbD::ASMTAllowRotation::storeOnTimeSeries(std::ofstream& os)
{
	os << "AllowRotationSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
