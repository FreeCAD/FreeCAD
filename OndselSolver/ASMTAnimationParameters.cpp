/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTAnimationParameters.h"

using namespace MbD;

void MbD::ASMTAnimationParameters::parseASMT(std::vector<std::string>& lines)
{
	//int nframe, icurrent, istart, iend, framesPerSecond;
	//bool isForward;
	size_t pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	assert(lines[0] == (leadingTabs + "nframe"));
	lines.erase(lines.begin());
	nframe = readInt(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "icurrent"));
	lines.erase(lines.begin());
	icurrent = readInt(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "istart"));
	lines.erase(lines.begin());
	istart = readInt(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "iend"));
	lines.erase(lines.begin());
	iend = readInt(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "isForward"));
	lines.erase(lines.begin());
	isForward = readBool(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "framesPerSecond"));
	lines.erase(lines.begin());
	framesPerSecond = readInt(lines[0]);
	lines.erase(lines.begin());

}
