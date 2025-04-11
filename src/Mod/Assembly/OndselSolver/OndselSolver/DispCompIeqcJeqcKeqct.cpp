/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DispCompIeqcJeqcKeqct.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"

using namespace MbD;

DispCompIeqcJeqcKeqct::DispCompIeqcJeqcKeqct()
{
}

DispCompIeqcJeqcKeqct::DispCompIeqcJeqcKeqct(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, size_t axisk) : DispCompIeqcJeqcKeqc(frmi, frmj, frmk, axisk)
{
}

void DispCompIeqcJeqcKeqct::initialize()
{
	DispCompIeqcJeqcKeqc::initialize();
	ppriIeJeKepXIpt = std::make_shared<FullRow<double>>(3);
	ppriIeJeKepEIpt = std::make_shared<FullRow<double>>(4);
	ppriIeJeKepXJpt = std::make_shared<FullRow<double>>(3);
	ppriIeJeKepEJpt = std::make_shared<FullRow<double>>(4);
	ppriIeJeKepEKpt = std::make_shared<FullRow<double>>(4);
}

void MbD::DispCompIeqcJeqcKeqct::initializeGlobally()
{
	//Do nothing.
}

void DispCompIeqcJeqcKeqct::calcPostDynCorrectorIteration()
{
	//"ppAjOIepEKpEK is not longer constant and must be set before any calculation."
	auto efrmKqc = std::static_pointer_cast<EndFrameqc>(efrmK);
	ppAjOKepEKpEK = efrmKqc->ppAjOepEpE(axisK);
	DispCompIeqcJeqcKeqc::calcPostDynCorrectorIteration();
}

void DispCompIeqcJeqcKeqct::preVelIC()
{
	Item::preVelIC();
	auto pAjOKept = std::static_pointer_cast<EndFrameqct>(efrmK)->pAjOept(axisK);
	priIeJeKept = pAjOKept->dot(rIeJeO);
}

double DispCompIeqcJeqcKeqct::pvaluept()
{
	return priIeJeKept;
}

FRowDsptr DispCompIeqcJeqcKeqct::ppvaluepXIpt()
{
	return ppriIeJeKepXIpt;
}

FRowDsptr DispCompIeqcJeqcKeqct::ppvaluepEIpt()
{
	return ppriIeJeKepEIpt;
}

FRowDsptr DispCompIeqcJeqcKeqct::ppvaluepEKpt()
{
	return ppriIeJeKepEKpt;
}

FRowDsptr DispCompIeqcJeqcKeqct::ppvaluepXJpt()
{
	return ppriIeJeKepXJpt;
}

FRowDsptr DispCompIeqcJeqcKeqct::ppvaluepEJpt()
{
	return ppriIeJeKepEJpt;
}

double DispCompIeqcJeqcKeqct::ppvalueptpt()
{
	return ppriIeJeKeptpt;
}

void DispCompIeqcJeqcKeqct::preAccIC()
{
	Item::preAccIC();
	auto pAjOKept = std::static_pointer_cast<EndFrameqct>(efrmK)->pAjOept(axisK);
	auto ppAjOKepEKTpt = std::static_pointer_cast<EndFrameqct>(efrmK)->ppAjOepETpt(axisK);
	auto ppAjOKeptpt = std::static_pointer_cast<EndFrameqct>(efrmK)->ppAjOeptpt(axisK);
	auto prIeJeOpEIT = std::static_pointer_cast<EndFrameqc>(frmI)->prOeOpE->transpose()->negated();
	auto prIeJeOpEJT = std::static_pointer_cast<EndFrameqc>(frmJ)->prOeOpE->transpose();
	for (size_t i = 0; i < 3; i++)
	{
		ppriIeJeKepXIpt->atiput(i, -(pAjOKept->at(i)));
		ppriIeJeKepXJpt->atiput(i, pAjOKept->at(i));
	}
	for (size_t i = 0; i < 4; i++)
	{
		ppriIeJeKepEIpt->atiput(i, pAjOKept->dot(prIeJeOpEIT->at(i)));
		ppriIeJeKepEJpt->atiput(i, pAjOKept->dot(prIeJeOpEJT->at(i)));
		ppriIeJeKepEKpt->atiput(i, ppAjOKepEKTpt->at(i)->dot(rIeJeO));
	}
	ppriIeJeKeptpt = ppAjOKeptpt->dot(rIeJeO);
}
