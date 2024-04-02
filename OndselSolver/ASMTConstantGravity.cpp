/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTConstantGravity.h"
#include "ASMTAssembly.h"
#include "Units.h"
#include "ConstantGravity.h"
#include "System.h"
#include "Part.h"

using namespace MbD;

std::shared_ptr<ASMTConstantGravity> MbD::ASMTConstantGravity::With()
{
	auto asmt = std::make_shared<ASMTConstantGravity>();
	asmt->initialize();
	return asmt;
}

void MbD::ASMTConstantGravity::parseASMT(std::vector<std::string>& lines)
{
	g = readColumnOfDoubles(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTConstantGravity::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	auto mbdGravity = CREATE<ConstantGravity>::With();
	mbdObject = mbdGravity;
	mbdGravity->gXYZ = g->times(1.0 / mbdUnits->acceleration);
	mbdSys->addForceTorque(mbdGravity);
}

FColDsptr MbD::ASMTConstantGravity::getg()
{
	return g;
}

void MbD::ASMTConstantGravity::setg(FColDsptr gravity)
{
	g = gravity;
}

void MbD::ASMTConstantGravity::setg(double a, double b, double c)
{
	g = std::make_shared<FullColumn<double>>(ListD{ a, b, c });
}

void MbD::ASMTConstantGravity::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "ConstantGravity");
	storeOnLevelArray(os, level + 1, *g);
}
