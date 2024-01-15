/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DistIeqcJec.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::DistIeqcJec::DistIeqcJec()
{
}

MbD::DistIeqcJec::DistIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj) : DistIecJec(frmi, frmj)
{
}

void MbD::DistIeqcJec::calcPrivate()
{
	DistIecJec::calcPrivate();
	if (rIeJe == 0.0) return;
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto& mprIeJeOpEI = frmIeqc->prOeOpE;
	mprIeJeOpEIT = mprIeJeOpEI->transpose();
	auto& mpprIeJeOpEIpEI = frmIeqc->pprOeOpEpE;
	auto muIeJeOT = muIeJeO->transpose();
	prIeJepXI = muIeJeOT;
	prIeJepEI = muIeJeOT->timesFullMatrix(mprIeJeOpEI);
	for (size_t i = 0; i < 3; i++)
	{
		auto& pprIeJepXIipXI = pprIeJepXIpXI->at(i);
		auto& prIeJepXIi = prIeJepXI->at(i);
		for (size_t j = 0; j < 3; j++)
		{
			auto element = (i == j) ? 1.0 : 0.0;
			element -= prIeJepXIi * prIeJepXI->at(j);
			pprIeJepXIipXI->atiput(j, element / rIeJe);
		}
	}
	pprIeJepXIpEI = std::make_shared<FullMatrix<double>>(3, 4);
	for (size_t i = 0; i < 3; i++)
	{
		auto& pprIeJepXIipEI = pprIeJepXIpEI->at(i);
		auto& prIeJepXIi = prIeJepXI->at(i);
		auto& mprIeJeOipEI = mprIeJeOpEI->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto element = mprIeJeOipEI->at(j) - prIeJepXIi * prIeJepEI->at(j);
			pprIeJepXIipEI->atiput(j, element / rIeJe);
		}
	}
	pprIeJepEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
	for (size_t i = 0; i < 4; i++)
	{
		auto& pprIeJepEIipEI = pprIeJepEIpEI->at(i);
		auto& prIeJepEIi = prIeJepEI->at(i);
		auto& mpprIeJeOpEIipEI = mpprIeJeOpEIpEI->at(i);
		auto& mprIeJeOpEIiT = mprIeJeOpEIT->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto element = mprIeJeOpEIiT->dot(mprIeJeOpEIT->at(j))
                           - mpprIeJeOpEIipEI->at(j)->dot(rIeJeO) - prIeJepEIi * prIeJepEI->at(j);
			pprIeJepEIipEI->atiput(j, element / rIeJe);
		}
	}
}

void MbD::DistIeqcJec::initialize()
{
	DistIecJec::initialize();
	prIeJepXI = std::make_shared<FullRow<double>>(3);
	prIeJepEI = std::make_shared<FullRow<double>>(4);
	pprIeJepXIpXI = std::make_shared<FullMatrix<double>>(3, 3);
	pprIeJepXIpEI = std::make_shared<FullMatrix<double>>(3, 4);
	pprIeJepEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

FMatDsptr MbD::DistIeqcJec::ppvaluepEIpEI()
{
	return pprIeJepEIpEI;
}

FMatDsptr MbD::DistIeqcJec::ppvaluepXIpEI()
{
	return pprIeJepXIpEI;
}

FMatDsptr MbD::DistIeqcJec::ppvaluepXIpXI()
{
	return pprIeJepXIpXI;
}

FRowDsptr MbD::DistIeqcJec::pvaluepEI()
{
	return prIeJepEI;
}

FRowDsptr MbD::DistIeqcJec::pvaluepXI()
{
	return prIeJepXI;
}
