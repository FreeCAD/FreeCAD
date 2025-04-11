/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DispCompIeqcJecKeqc.h"
#include "EndFrameqc.h"

using namespace MbD;

DispCompIeqcJecKeqc::DispCompIeqcJecKeqc()
{
}

DispCompIeqcJecKeqc::DispCompIeqcJecKeqc(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, size_t axisk) : DispCompIecJecKeqc(frmi, frmj, frmk, axisk)
{
}

void DispCompIeqcJecKeqc::initialize()
{
	DispCompIecJecKeqc::initialize();
	priIeJeKepXI = std::make_shared<FullRow<double>>(3);
	priIeJeKepEI = std::make_shared<FullRow<double>>(4);
	ppriIeJeKepEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
	ppriIeJeKepXIpEK = std::make_shared<FullMatrix<double>>(3, 4);
	ppriIeJeKepEIpEK = std::make_shared<FullMatrix<double>>(4, 4);
}

void DispCompIeqcJecKeqc::calcPostDynCorrectorIteration()
{
	DispCompIecJecKeqc::calcPostDynCorrectorIteration();
	auto frmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto mprIeJeOpEIT = frmIqc->prOeOpE->transpose();
	auto& mpprIeJeOpEIpEI = frmIqc->pprOeOpEpE;
	for (size_t i = 0; i < 3; i++)
	{
		priIeJeKepXI->at(i) = 0.0 - (aAjOKe->at(i));
	}
	for (size_t i = 0; i < 4; i++)
	{
		priIeJeKepEI->at(i) = 0.0 - (aAjOKe->dot(mprIeJeOpEIT->at(i)));
	}
	for (size_t i = 0; i < 3; i++)
	{
		auto& ppriIeJeKepXIipEK = ppriIeJeKepXIpEK->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			ppriIeJeKepXIipEK->at(j) = 0.0 - (pAjOKepEKT->at(j)->at(i));
		}
	}
	for (size_t i = 0; i < 4; i++)
	{
		auto& mpprIeJeOpEIipEI = mpprIeJeOpEIpEI->at(i);
		auto& ppriIeJeKepEIipEI = ppriIeJeKepEIpEI->at(i);
		ppriIeJeKepEIipEI->at(i) = 0.0 - (aAjOKe->dot(mpprIeJeOpEIipEI->at(i)));
		for (size_t j = 0; j < 4; j++)
		{
			auto ppriIeJeKepEIipEIj = 0.0 - (aAjOKe->dot(mpprIeJeOpEIipEI->at(j)));
			ppriIeJeKepEIipEI->at(j) = ppriIeJeKepEIipEIj;
			ppriIeJeKepEIpEI->at(j)->at(i) = ppriIeJeKepEIipEIj;
		}
	}
	for (size_t i = 0; i < 4; i++)
	{
		auto& mprIeJeOpEITi = mprIeJeOpEIT->at(i);
		auto& ppriIeJeKepEIipEK = ppriIeJeKepEIpEK->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			ppriIeJeKepEIipEK->at(j) = 0.0 - (pAjOKepEKT->at(j)->dot(mprIeJeOpEITi));
		}
	}
}

FRowDsptr DispCompIeqcJecKeqc::pvaluepXI()
{
	return priIeJeKepXI;
}

FRowDsptr DispCompIeqcJecKeqc::pvaluepEI()
{
	return priIeJeKepEI;
}

FMatDsptr DispCompIeqcJecKeqc::ppvaluepXIpEK()
{
	return ppriIeJeKepXIpEK;
}

FMatDsptr DispCompIeqcJecKeqc::ppvaluepEIpEK()
{
	return ppriIeJeKepEIpEK;
}

FMatDsptr DispCompIeqcJecKeqc::ppvaluepEIpEI()
{
	return ppriIeJeKepEIpEI;
}
