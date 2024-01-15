/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DispCompIeqctJeqcO.h"
#include "EndFrameqct.h"

using namespace MbD;

DispCompIeqctJeqcO::DispCompIeqctJeqcO()
{
}

DispCompIeqctJeqcO::DispCompIeqctJeqcO(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis) : DispCompIeqcJeqcO(frmi, frmj, axis)
{
}

void DispCompIeqctJeqcO::initializeGlobally()
{
	//ToDo: Check why not using super classes.
	ppriIeJeOpEJpEJ = std::static_pointer_cast<EndFrameqct>(frmJ)->ppriOeOpEpE(axis);
}

FRowDsptr MbD::DispCompIeqctJeqcO::ppvaluepEIpt()
{
	return ppriIeJeOpEIpt;
}

double MbD::DispCompIeqctJeqcO::ppvalueptpt()
{
	return ppriIeJeOptpt;
}

void DispCompIeqctJeqcO::calcPostDynCorrectorIteration()
{
	//"ppriIeJeOpEIpEI is not a constant now."
	DispCompIeqcJeqcO::calcPostDynCorrectorIteration();
	ppriIeJeOpEIpEI = std::static_pointer_cast<EndFrameqct>(frmI)->ppriOeOpEpE(axis)->negated();
}

void DispCompIeqctJeqcO::preVelIC()
{
	Item::preVelIC();
	priIeJeOpt = -(std::static_pointer_cast<EndFrameqct>(frmI)->priOeOpt(axis));
}

double DispCompIeqctJeqcO::pvaluept()
{
	return priIeJeOpt;
}

void DispCompIeqctJeqcO::preAccIC()
{
	Item::preAccIC();
	ppriIeJeOpEIpt = (std::static_pointer_cast<EndFrameqct>(frmI)->ppriOeOpEpt(axis))->negated();
	ppriIeJeOptpt = -(std::static_pointer_cast<EndFrameqct>(frmI)->ppriOeOptpt(axis));
}
