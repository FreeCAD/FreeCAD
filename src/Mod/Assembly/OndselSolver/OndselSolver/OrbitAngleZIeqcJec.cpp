/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "OrbitAngleZIeqcJec.h"
#include "CREATE.h"
#include "DispCompIeqcJecIe.h"

using namespace MbD;

MbD::OrbitAngleZIeqcJec::OrbitAngleZIeqcJec()
{
}

MbD::OrbitAngleZIeqcJec::OrbitAngleZIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj) : OrbitAngleZIecJec(frmi, frmj)
{
}

void MbD::OrbitAngleZIeqcJec::calc_ppthezpEIpEI()
{
	auto pxpEI = xIeJeIe->pvaluepEI();
	auto pypEI = yIeJeIe->pvaluepEI();
	auto ppxpEIpEI = xIeJeIe->ppvaluepEIpEI();
	auto ppypEIpEI = yIeJeIe->ppvaluepEIpEI();
	for (size_t i = 0; i < 4; i++)
	{
		auto ppthezpEIpEIi = ppthezpEIpEI->at(i);
		auto ppxpEIpEIi = ppxpEIpEI->at(i);
		auto ppypEIpEIi = ppypEIpEI->at(i);
		auto pxpEIi = pxpEI->at(i);
		auto pypEIi = pypEI->at(i);
		for (size_t j = i; j < 4; j++)
		{
			auto pxpEIj = pxpEI->at(j);
			auto pypEIj = pypEI->at(j);
			auto term1 = (pxpEIi * pxpEIj - (pypEIi * pypEIj)) * twoCosSinOverSSqSq;
			auto term2 = ppypEIpEIi->at(j) * cosOverSSq - (ppxpEIpEIi->at(j) * sinOverSSq);
			auto term3 = (pypEIi * pxpEIj + (pxpEIi * pypEIj)) * dSqOverSSqSq;
			auto ppthezpEIpEIij = term1 + term2 + term3;
			ppthezpEIpEIi->atiput(j, ppthezpEIpEIij);
			if (i < j) ppthezpEIpEI->atijput(j, i, ppthezpEIpEIij);
		}
	}
}

void MbD::OrbitAngleZIeqcJec::calc_ppthezpXIpEI()
{
	auto pxpXI = xIeJeIe->pvaluepXI();
	auto pypXI = yIeJeIe->pvaluepXI();
	auto pxpEI = xIeJeIe->pvaluepEI();
	auto pypEI = yIeJeIe->pvaluepEI();
	auto ppxpXIpEI = xIeJeIe->ppvaluepXIpEI();
	auto ppypXIpEI = yIeJeIe->ppvaluepXIpEI();
	for (size_t i = 0; i < 3; i++)
	{
		auto ppthezpXIpEIi = ppthezpXIpEI->at(i);
		auto ppxpXIpEIi = ppxpXIpEI->at(i);
		auto ppypXIpEIi = ppypXIpEI->at(i);
		auto pxpXIi = pxpXI->at(i);
		auto pypXIi = pypXI->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto pxpEIj = pxpEI->at(j);
			auto pypEIj = pypEI->at(j);
			auto term1 = (pxpXIi * pxpEIj - (pypXIi * pypEIj)) * twoCosSinOverSSqSq;
			auto term2 = ppypXIpEIi->at(j) * cosOverSSq - (ppxpXIpEIi->at(j) * sinOverSSq);
			auto term3 = (pypXIi * pxpEIj + (pxpXIi * pypEIj)) * dSqOverSSqSq;
			ppthezpXIpEIi->atiput(j, term1 + term2 + term3);
		}
	}
}

void MbD::OrbitAngleZIeqcJec::calc_ppthezpXIpXI()
{
	//ppxpXIpXI = 0
	//ppypXIpXI = 0

	auto pxpXI = xIeJeIe->pvaluepXI();
	auto pypXI = yIeJeIe->pvaluepXI();
	for (size_t i = 0; i < 3; i++)
	{
		auto ppthezpXIpXIi = ppthezpXIpXI->at(i);
		auto pxpXIi = pxpXI->at(i);
		auto pypXIi = pypXI->at(i);
		for (size_t j = 0; j < 3; j++)
		{
			auto pxpXIj = pxpXI->at(j);
			auto pypXIj = pypXI->at(j);
			auto term1 = (pxpXIi * pxpXIj - (pypXIi * pypXIj)) * twoCosSinOverSSqSq;
			auto term3 = (pypXIi * pxpXIj + (pxpXIi * pypXIj)) * dSqOverSSqSq;
			ppthezpXIpXIi->atiput(j, term1 + term3);
		}
	}
}

void MbD::OrbitAngleZIeqcJec::calc_pthezpEI()
{
	auto pxpEI = xIeJeIe->pvaluepEI();
	auto pypEI = yIeJeIe->pvaluepEI();
	for (size_t i = 0; i < 4; i++)
	{
		pthezpEI->atiput(i, pypEI->at(i) * cosOverSSq - (pxpEI->at(i) * sinOverSSq));
	}
}

void MbD::OrbitAngleZIeqcJec::calc_pthezpXI()
{
	auto pxpXI = xIeJeIe->pvaluepXI();
	auto pypXI = yIeJeIe->pvaluepXI();
	for (size_t i = 0; i < 3; i++)
	{
		pthezpXI->atiput(i, pypXI->at(i) * cosOverSSq - (pxpXI->at(i) * sinOverSSq));
	}
}

void MbD::OrbitAngleZIeqcJec::calcPostDynCorrectorIteration()
{
	OrbitAngleZIecJec::calcPostDynCorrectorIteration();
	this->calc_pthezpXI();
	this->calc_pthezpEI();
	this->calc_ppthezpXIpXI();
	this->calc_ppthezpXIpEI();
	this->calc_ppthezpEIpEI();
}

void MbD::OrbitAngleZIeqcJec::init_xyIeJeIe()
{
	xIeJeIe = CREATE<DispCompIeqcJecIe>::With(frmI, frmJ, 0);
	yIeJeIe = CREATE<DispCompIeqcJecIe>::With(frmI, frmJ, 1);
}

void MbD::OrbitAngleZIeqcJec::initialize()
{
	OrbitAngleZIecJec::initialize();
	pthezpXI = std::make_shared<FullRow<double>>(3);
	pthezpEI = std::make_shared<FullRow<double>>(4);
	ppthezpXIpXI = std::make_shared<FullMatrix<double>>(3, 3);
	ppthezpXIpEI = std::make_shared<FullMatrix<double>>(3, 4);
	ppthezpEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

FMatDsptr MbD::OrbitAngleZIeqcJec::ppvaluepEIpEI()
{
	return ppthezpEIpEI;
}

FMatDsptr MbD::OrbitAngleZIeqcJec::ppvaluepXIpEI()
{
	return ppthezpXIpEI;
}

FMatDsptr MbD::OrbitAngleZIeqcJec::ppvaluepXIpXI()
{
	return ppthezpXIpXI;
}

FRowDsptr MbD::OrbitAngleZIeqcJec::pvaluepEI()
{
	return pthezpEI;
}

FRowDsptr MbD::OrbitAngleZIeqcJec::pvaluepXI()
{
	return pthezpXI;
}
