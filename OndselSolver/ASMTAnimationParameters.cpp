/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTAnimationParameters.h"

using namespace MbD;

std::shared_ptr<ASMTAnimationParameters> MbD::ASMTAnimationParameters::With()
{
	auto asmt = std::make_shared<ASMTAnimationParameters>();
	asmt->initialize();
	return asmt;
}

void MbD::ASMTAnimationParameters::parseASMT(std::vector<std::string>& lines)
{
	//size_t nframe, icurrent, istart, iend, framesPerSecond;
	//bool isForward;
	auto pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	assert(lines[0] == (leadingTabs + "nframe"));
	lines.erase(lines.begin());
	nframe = readSize_t(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "icurrent"));
	lines.erase(lines.begin());
	icurrent = readSize_t(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "istart"));
	lines.erase(lines.begin());
	istart = readSize_t(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "iend"));
	lines.erase(lines.begin());
	iend = readSize_t(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "isForward"));
	lines.erase(lines.begin());
	isForward = readBool(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "framesPerSecond"));
	lines.erase(lines.begin());
	framesPerSecond = readSize_t(lines[0]);
	lines.erase(lines.begin());

}

void MbD::ASMTAnimationParameters::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "AnimationParameters");
	storeOnLevelString(os, level + 1, "nframe");
	storeOnLevelSize_t(os, level + 2, nframe);
	storeOnLevelString(os, level + 1, "icurrent");
	storeOnLevelSize_t(os, level + 2, icurrent);
	storeOnLevelString(os, level + 1, "istart");
	storeOnLevelSize_t(os, level + 2, istart);
	storeOnLevelString(os, level + 1, "iend");
	storeOnLevelSize_t(os, level + 2, iend);
	storeOnLevelString(os, level + 1, "isForward");
	storeOnLevelBool(os, level + 2, isForward);
	storeOnLevelString(os, level + 1, "framesPerSecond");
	storeOnLevelSize_t(os, level + 2, framesPerSecond);
}
