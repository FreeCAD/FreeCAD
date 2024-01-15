/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DispCompIeqcJecO.h"
#include "EndFrameqc.h"

using namespace MbD;

DispCompIeqcJecO::DispCompIeqcJecO()
{
}

DispCompIeqcJecO::DispCompIeqcJecO(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis) : DispCompIecJecO(frmi, frmj, axis)
{
}

void DispCompIeqcJecO::initializeGlobally()
{
	priIeJeOpXI = std::make_shared<FullRow<double>>(3, 0.0);
	priIeJeOpXI->at(axis) = -1.0;
	ppriIeJeOpEIpEI = std::static_pointer_cast<EndFrameqc>(frmI)->ppriOeOpEpE(axis)->negated();
}

FMatDsptr MbD::DispCompIeqcJecO::ppvaluepEIpEI()
{
	return ppriIeJeOpEIpEI;
}

FRowDsptr MbD::DispCompIeqcJecO::pvaluepEI()
{
	return priIeJeOpEI;
}

FRowDsptr MbD::DispCompIeqcJecO::pvaluepXI()
{
	return priIeJeOpXI;
}

void DispCompIeqcJecO::calcPostDynCorrectorIteration()
{
	DispCompIecJecO::calcPostDynCorrectorIteration();
	priIeJeOpEI = std::static_pointer_cast<EndFrameqc>(frmI)->priOeOpE(axis)->negated();
}
