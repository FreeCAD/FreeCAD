/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTSimulationParameters.h"

using namespace MbD;

std::shared_ptr<ASMTSimulationParameters> MbD::ASMTSimulationParameters::With()
{
	auto asmt = std::make_shared<ASMTSimulationParameters>();
	asmt->initialize();
	return asmt;
}

void MbD::ASMTSimulationParameters::parseASMT(std::vector<std::string>& lines)
{
	//tstart, tend, hmin, hmax, hout, errorTol;

	auto pos = lines[0].find_first_not_of("\t");
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

void MbD::ASMTSimulationParameters::settstart(double t)
{
	tstart = t;
}

void MbD::ASMTSimulationParameters::settend(double t)
{
	tend = t;
}

void MbD::ASMTSimulationParameters::sethmin(double h)
{
	hmin = h;
}

void MbD::ASMTSimulationParameters::sethmax(double h)
{
	hmax = h;
}

void MbD::ASMTSimulationParameters::sethout(double h)
{
	hout = h;
}

void MbD::ASMTSimulationParameters::seterrorTol(double tol)
{
	errorTol = tol;
    errorTolPosKine = tol;
	errorTolAccKine = tol;
	corAbsTol = tol;
	corRelTol = tol;
	intAbsTol = tol;
    intRelTol = tol;
}

void MbD::ASMTSimulationParameters::setmaxIter(size_t maxIter)
{
	iterMaxPosKine = maxIter;
	iterMaxAccKine = maxIter;
}

void MbD::ASMTSimulationParameters::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "SimulationParameters");
	storeOnLevelString(os, level + 1, "tstart");
	storeOnLevelDouble(os, level + 2, tstart);
	storeOnLevelString(os, level + 1, "tend");
	storeOnLevelDouble(os, level + 2, tend);
	storeOnLevelString(os, level + 1, "hmin");
	storeOnLevelDouble(os, level + 2, hmin);
	storeOnLevelString(os, level + 1, "hmax");
	storeOnLevelDouble(os, level + 2, hmax);
	storeOnLevelString(os, level + 1, "hout");
	storeOnLevelDouble(os, level + 2, hout);
	storeOnLevelString(os, level + 1, "errorTol");
	storeOnLevelDouble(os, level + 2, errorTol);
}
