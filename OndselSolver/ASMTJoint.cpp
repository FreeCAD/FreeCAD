/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTJoint.h"

using namespace MbD;

void MbD::ASMTJoint::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	readMarkerI(lines);
	readMarkerJ(lines);
}

void MbD::ASMTJoint::readJointSeries(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	std::string substr = "JointSeries";
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

void MbD::ASMTJoint::storeOnLevel(std::ofstream& os, int level)
{
	assert(false);
}

void MbD::ASMTJoint::storeOnTimeSeries(std::ofstream& os)
{
	assert(false);
}
