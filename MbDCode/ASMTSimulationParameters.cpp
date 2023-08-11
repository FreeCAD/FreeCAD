/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTSimulationParameters.h"

using namespace MbD;

void MbD::ASMTSimulationParameters::parseASMT(std::vector<std::string>& lines)
{
	//tstart, tend, hmin, hmax, hout, errorTol;

	size_t pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	assert(lines[0] == (leadingTabs + "tstart"));
	lines.erase(lines.begin());
	tstart = readDouble(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "tend"));
	lines.erase(lines.begin());
	tend = readDouble(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "hmin"));
	lines.erase(lines.begin());
	hmin = readDouble(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "hmax"));
	lines.erase(lines.begin());
	hmax = readDouble(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "hout"));
	lines.erase(lines.begin());
	hout = readDouble(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "errorTol"));
	lines.erase(lines.begin());
	errorTol = readDouble(lines[0]);
	lines.erase(lines.begin());

}
