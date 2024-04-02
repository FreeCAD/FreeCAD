/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTMotion.h"
#include "Joint.h"

using namespace MbD;

void ASMTMotion::readMotionSeries(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	std::string substr = "tionSeries";
	auto pos = str.find(substr);
	assert(pos != std::string::npos);
	str.erase(0, pos + substr.length());
	auto seriesName = readString(str);
	assert(fullName("") == seriesName);
	lines.erase(lines.begin());
	readFXonIs(lines);
	readFYonIs(lines);
	readFZonIs(lines);
	readTXonIs(lines);
	readTYonIs(lines);
	readTZonIs(lines);
}

void ASMTMotion::initMarkers()
{
}

void ASMTMotion::storeOnLevel(std::ofstream&, size_t)
{
	assert(false);
}

void ASMTMotion::storeOnTimeSeries(std::ofstream&)
{
	assert(false);
}

void MbD::ASMTMotion::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTConstraintSet::createMbD(mbdSys, mbdUnits);
	auto mbdJt = std::static_pointer_cast<Joint>(mbdObject);
	mbdSys->addJoint(mbdJt);
}
