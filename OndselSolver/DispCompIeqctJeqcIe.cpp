/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DispCompIeqctJeqcIe.h"
#include "EndFrameqct.h"

using namespace MbD;

MbD::DispCompIeqctJeqcIe::DispCompIeqctJeqcIe()
{
}

MbD::DispCompIeqctJeqcIe::DispCompIeqctJeqcIe(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis) : DispCompIeqcJeqcIe(frmi, frmj, axis)
{
	ppriIeJeIepXIpt = std::make_shared<FullRow<double>>(3);
	ppriIeJeIepEIpt = std::make_shared<FullRow<double>>(4);
	ppriIeJeIepXJpt = std::make_shared<FullRow<double>>(3);
	ppriIeJeIepEJpt = std::make_shared<FullRow<double>>(4);
}

void MbD::DispCompIeqctJeqcIe::calc_ppvaluepEIpt()
{
	auto frmIeqct = std::static_pointer_cast<EndFrameqct>(frmI);
	auto pAjOIept = frmIeqct->pAjOept(axis);
	auto ppAjOIepEITpt = frmIeqct->ppAjOepETpt(axis);
	auto mprIeJeOpEIT = frmIeqct->prOeOpE->transpose();
	auto mpprIeJeOpEITpt = frmIeqct->pprOeOpEpt->transpose();
	auto mprIeJeOpt = frmIeqct->prOeOpt;
	for (size_t i = 0; i < 4; i++)
	{
		auto ppAjOIepEITpti = ppAjOIepEITpt->at(i);
		auto pAjOIepEITi = pAjOIepEIT->at(i);
		auto mprIeJeOpEITi = mprIeJeOpEIT->at(i);
		auto mpprIeJeOpEITpti = mpprIeJeOpEITpt->at(i);
		auto ppriIeJeIepEIpti = ppAjOIepEITpti->dot(rIeJeO) - pAjOIepEITi->dot(mprIeJeOpt) -
                pAjOIept->dot(mprIeJeOpEITi) -
                                aAjOIe->dot(mpprIeJeOpEITpti);
		ppriIeJeIepEIpt->atiput(i, ppriIeJeIepEIpti);
	}
}

void MbD::DispCompIeqctJeqcIe::calc_ppvaluepEJpt()
{
	auto frmIeqct = std::static_pointer_cast<EndFrameqct>(frmI);
	auto frmJeqct = std::static_pointer_cast<EndFrameqct>(frmJ);
	auto pAjOIept = frmIeqct->pAjOept(axis);
	auto prIeJeOpEJT = frmJeqct->prOeOpE->transpose();
	for (size_t i = 0; i < 4; i++)
	{
		ppriIeJeIepEJpt->atiput(i, pAjOIept->dot(prIeJeOpEJT->at(i)));
	}
}

void MbD::DispCompIeqctJeqcIe::calc_ppvalueptpt()
{
	auto frmIeqct = std::static_pointer_cast<EndFrameqct>(frmI);
	auto pAjOIept = frmIeqct->pAjOept(axis);
	auto ppAjOIeptpt = frmIeqct->ppAjOeptpt(axis);
	auto mprIeJeOpt = frmIeqct->prOeOpt;
	auto mpprIeJeOptpt = frmIeqct->pprOeOptpt;
	ppriIeJeIeptpt = ppAjOIeptpt->dot(rIeJeO) - pAjOIept->dot(mprIeJeOpt) - pAjOIept->dot(mprIeJeOpt) -
                     aAjOIe->dot(mpprIeJeOptpt);
}

void MbD::DispCompIeqctJeqcIe::calc_ppvaluepXIpt()
{
	auto frmIeqct = std::static_pointer_cast<EndFrameqct>(frmI);
	auto pAjOIept = frmIeqct->pAjOept(axis);
	for (size_t i = 0; i < 3; i++)
	{
		ppriIeJeIepXIpt->atiput(i, -pAjOIept->at(i));
	}
}

void MbD::DispCompIeqctJeqcIe::calc_ppvaluepXJpt()
{
	auto frmIeqct = std::static_pointer_cast<EndFrameqct>(frmI);
	auto pAjOIept = frmIeqct->pAjOept(axis);
	for (size_t i = 0; i < 3; i++)
	{
		ppriIeJeIepXJpt->atiput(i, pAjOIept->at(i));
	}
}

void MbD::DispCompIeqctJeqcIe::calc_pvaluept()
{
	auto frmIeqct = std::static_pointer_cast<EndFrameqct>(frmI);
	auto pAjOIept = frmIeqct->pAjOept(axis);
	auto mprIeJeOpt = frmIeqct->prOeOpt;
	priIeJeIept = pAjOIept->dot(rIeJeO) - aAjOIe->dot(mprIeJeOpt);
}

void MbD::DispCompIeqctJeqcIe::calcPostDynCorrectorIteration()
{
	//"ppAjOIepEIpEI is not longer constant and must be set before any calculation."
	auto frmIeqct = std::static_pointer_cast<EndFrameqct>(frmI);
	ppAjOIepEIpEI = frmIeqct->ppAjOepEpE(axis);
	DispCompIeqcJeqcIe::calcPostDynCorrectorIteration();
}

void MbD::DispCompIeqctJeqcIe::initialize()
{
	DispCompIeqcJeqcIe::initialize();
	ppriIeJeIepXIpt = std::make_shared<FullRow<double>>(3);
	ppriIeJeIepEIpt = std::make_shared<FullRow<double>>(4);
	ppriIeJeIepXJpt = std::make_shared<FullRow<double>>(3);
	ppriIeJeIepEJpt = std::make_shared<FullRow<double>>(4);
}

void MbD::DispCompIeqctJeqcIe::initializeGlobally()
{
	//"Do nothing."
}

void MbD::DispCompIeqctJeqcIe::preAccIC()
{
	DispCompIeqcJeqcIe::preAccIC();
	calc_ppvaluepXIpt();
	calc_ppvaluepEIpt();
	calc_ppvaluepXJpt();
	calc_ppvaluepEJpt();
	calc_ppvalueptpt();
}

void MbD::DispCompIeqctJeqcIe::preVelIC()
{
	DispCompIeqcJeqcIe::preVelIC();
	calc_pvaluept();
}

FRowDsptr MbD::DispCompIeqctJeqcIe::ppvaluepEIpt()
{
	return ppriIeJeIepEIpt;
}

FRowDsptr MbD::DispCompIeqctJeqcIe::ppvaluepEJpt()
{
	return ppriIeJeIepEJpt;
}

double MbD::DispCompIeqctJeqcIe::ppvalueptpt()
{
	return ppriIeJeIeptpt;
}

FRowDsptr MbD::DispCompIeqctJeqcIe::ppvaluepXIpt()
{
	return ppriIeJeIepXIpt;
}

FRowDsptr MbD::DispCompIeqctJeqcIe::ppvaluepXJpt()
{
	return ppriIeJeIepXJpt;
}

double MbD::DispCompIeqctJeqcIe::pvaluept()
{
	return priIeJeIept;
}
