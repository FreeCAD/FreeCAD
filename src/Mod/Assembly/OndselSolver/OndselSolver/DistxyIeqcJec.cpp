/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DistxyIeqcJec.h"
#include "CREATE.h"
#include "DispCompIeqcJecIe.h"

using namespace MbD;

MbD::DistxyIeqcJec::DistxyIeqcJec()
{
}

MbD::DistxyIeqcJec::DistxyIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj) : DistxyIecJec(frmi, frmj)
{
}

void MbD::DistxyIeqcJec::calc_ppdistxypEIpEI()
{
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	auto pxpEI = xIeJeIe->pvaluepEI();
	auto pypEI = yIeJeIe->pvaluepEI();
	auto ppxpEIpEI = xIeJeIe->ppvaluepEIpEI();
	auto ppypEIpEI = yIeJeIe->ppvaluepEIpEI();
	for (size_t i = 0; i < 4; i++)
	{
		auto& ppdistxypEIpEIi = ppdistxypEIpEI->at(i);
		auto& pdistxypEIi = pdistxypEI->at(i);
		auto& ppxpEIpEIi = ppxpEIpEI->at(i);
		auto& ppypEIpEIi = ppypEIpEI->at(i);
		auto& pxpEIi = pxpEI->at(i);
		auto& pypEIi = pypEI->at(i);
		for (size_t j = i; j < 4; j++)
		{
			auto pdistxypEIj = pdistxypEI->at(j);
			auto pxpEIj = pxpEI->at(j);
			auto pypEIj = pypEI->at(j);
			auto term1 = -pdistxypEIi * pdistxypEIj;
			auto term2 = ppxpEIpEIi->at(j) * x + ppypEIpEIi->at(j) * y;
			auto term3 = pxpEIi * pxpEIj + pypEIi * pypEIj;
			auto ppdistxypEIpEIij = (term1 + term2 + term3) / distxy;
			ppdistxypEIpEIi->atiput(j, ppdistxypEIpEIij);
			if (i < j) ppdistxypEIpEI->atijput(j, i, ppdistxypEIpEIij);
		}
	}
}

void MbD::DistxyIeqcJec::calc_ppdistxypXIpEI()
{
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	auto pxpXI = xIeJeIe->pvaluepXI();
	auto pypXI = yIeJeIe->pvaluepXI();
	auto pxpEI = xIeJeIe->pvaluepEI();
	auto pypEI = yIeJeIe->pvaluepEI();
	auto ppxpXIpEI = xIeJeIe->ppvaluepXIpEI();
	auto ppypXIpEI = yIeJeIe->ppvaluepXIpEI();
	for (size_t i = 0; i < 3; i++)
	{
		auto& ppdistxypXIpEIi = ppdistxypXIpEI->at(i);
		auto& pdistxypXIi = pdistxypXI->at(i);
		auto& ppxpXIpEIi = ppxpXIpEI->at(i);
		auto& ppypXIpEIi = ppypXIpEI->at(i);
		auto& pxpXIi = pxpXI->at(i);
		auto& pypXIi = pypXI->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto& pdistxypEIj = pdistxypEI->at(j);
			auto& pxpEIj = pxpEI->at(j);
			auto& pypEIj = pypEI->at(j);
			auto term1 = -pdistxypXIi * pdistxypEIj;
			auto term2 = ppxpXIpEIi->at(j) * x + ppypXIpEIi->at(j) * y;
			auto term3 = pxpXIi * pxpEIj + pypXIi * pypEIj;
			auto ppdistxypXIpEIij = (term1 + term2 + term3) / distxy;
			ppdistxypXIpEIi->atiput(j, ppdistxypXIpEIij);
		}
	}
}

void MbD::DistxyIeqcJec::calc_ppdistxypXIpXI()
{
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	auto pxpXI = xIeJeIe->pvaluepXI();
	auto pypXI = yIeJeIe->pvaluepXI();
	auto ppxpXIpXI = xIeJeIe->ppvaluepXIpXI();
	auto ppypXIpXI = yIeJeIe->ppvaluepXIpXI();
	for (size_t i = 0; i < 3; i++)
	{
		auto& ppdistxypXIpXIi = ppdistxypXIpXI->at(i);
		auto& pdistxypXIi = pdistxypXI->at(i);
		auto& ppxpXIpXIi = ppxpXIpXI->at(i);
		auto& ppypXIpXIi = ppypXIpXI->at(i);
		auto& pxpXIi = pxpXI->at(i);
		auto& pypXIi = pypXI->at(i);
		for (size_t j = i; j < 3; j++)
		{
			auto pdistxypXIj = pdistxypXI->at(j);
			auto pxpXIj = pxpXI->at(j);
			auto pypXIj = pypXI->at(j);
			auto term1 = -pdistxypXIi * pdistxypXIj;
			auto term2 = ppxpXIpXIi->at(j) * x + ppypXIpXIi->at(j) * y;
			auto term3 = pxpXIi * pxpXIj + pypXIi * pypXIj;
			auto ppdistxypXIpXIij = (term1 + term2 + term3) / distxy;
			ppdistxypXIpXIi->atiput(j, ppdistxypXIpXIij);
			if (i < j) ppdistxypXIpXI->atijput(j, i, ppdistxypXIpXIij);
		}
	}
}

void MbD::DistxyIeqcJec::calc_pdistxypEI()
{
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	auto pxpEI = xIeJeIe->pvaluepEI();
	auto pypEI = yIeJeIe->pvaluepEI();
	for (size_t i = 0; i < 4; i++)
	{
		auto term = pxpEI->at(i) * x + pypEI->at(i) * y;
		pdistxypEI->atiput(i, term / distxy);
	}
}

void MbD::DistxyIeqcJec::calc_pdistxypXI()
{
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	auto pxpXI = xIeJeIe->pvaluepXI();
	auto pypXI = yIeJeIe->pvaluepXI();
	for (size_t i = 0; i < 3; i++)
	{
		auto term = pxpXI->at(i) * x + pypXI->at(i) * y;
		pdistxypXI->atiput(i, term / distxy);
	}
}

void MbD::DistxyIeqcJec::calcPostDynCorrectorIteration()
{
	DistxyIecJec::calcPostDynCorrectorIteration();
	this->calc_pdistxypXI();
	this->calc_pdistxypEI();
	this->calc_ppdistxypXIpXI();
	this->calc_ppdistxypXIpEI();
	this->calc_ppdistxypEIpEI();
}

void MbD::DistxyIeqcJec::init_xyIeJeIe()
{
	xIeJeIe = CREATE<DispCompIeqcJecIe>::With(frmI, frmJ, 0);
	yIeJeIe = CREATE<DispCompIeqcJecIe>::With(frmI, frmJ, 1);
}

void MbD::DistxyIeqcJec::initialize()
{
	DistxyIecJec::initialize();
	pdistxypXI = std::make_shared<FullRow<double>>(3);
	pdistxypEI = std::make_shared<FullRow<double>>(4);
	ppdistxypXIpXI = std::make_shared<FullMatrix<double>>(3, 3);
	ppdistxypXIpEI = std::make_shared<FullMatrix<double>>(3, 4);
	ppdistxypEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

FMatDsptr MbD::DistxyIeqcJec::ppvaluepEIpEI()
{
	return ppdistxypEIpEI;
}

FMatDsptr MbD::DistxyIeqcJec::ppvaluepXIpEI()
{
	return ppdistxypXIpEI;
}

FMatDsptr MbD::DistxyIeqcJec::ppvaluepXIpXI()
{
	return ppdistxypXIpXI;
}

FRowDsptr MbD::DistxyIeqcJec::pvaluepEI()
{
	return pdistxypEI;
}

FRowDsptr MbD::DistxyIeqcJec::pvaluepXI()
{
	return pdistxypXI;
}
