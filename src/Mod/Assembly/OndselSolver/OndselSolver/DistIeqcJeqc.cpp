/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DistIeqcJeqc.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::DistIeqcJeqc::DistIeqcJeqc()
{
}

MbD::DistIeqcJeqc::DistIeqcJeqc(EndFrmsptr frmi, EndFrmsptr frmj) : DistIeqcJec(frmi, frmj)
{
}

void MbD::DistIeqcJeqc::calcPrivate()
{
	DistIeqcJec::calcPrivate();
	if (rIeJe == 0.0) return;
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto& prIeJeOpEJ = frmJeqc->prOeOpE;
	auto prIeJeOpEJT = prIeJeOpEJ->transpose();
	auto& pprIeJeOpEJpEJ = frmJeqc->pprOeOpEpE;
	auto uIeJeOT = uIeJeO->transpose();
	prIeJepXJ = uIeJeOT;
	prIeJepEJ = uIeJeOT->timesFullMatrix(prIeJeOpEJ);
	for (size_t i = 0; i < 3; i++)
	{
		auto& pprIeJepXIipXJ = pprIeJepXIpXJ->at(i);
		auto& prIeJepXIi = prIeJepXI->at(i);
		for (size_t j = 0; j < 3; j++)
		{
			auto element = (i == j) ? -1.0 : 0.0;
			element -= prIeJepXIi * prIeJepXJ->at(j);
			pprIeJepXIipXJ->atiput(j, element / rIeJe);
		}
	}

	for (size_t i = 0; i < 4; i++)
	{
		auto& pprIeJepEIipXJ = pprIeJepEIpXJ->at(i);
		auto& prIeJepEIi = prIeJepEI->at(i);
		auto& mprIeJeOpEIiT = mprIeJeOpEIT->at(i);
		for (size_t j = 0; j < 3; j++)
		{
			auto element = 0.0 - mprIeJeOpEIiT->at(j) - prIeJepEIi * prIeJepXJ->at(j);
			pprIeJepEIipXJ->atiput(j, element / rIeJe);
		}
	}

	for (size_t i = 0; i < 3; i++)
	{
		auto& pprIeJepXJipXJ = pprIeJepXJpXJ->at(i);
		auto& prIeJepXJi = prIeJepXJ->at(i);
		for (size_t j = 0; j < 3; j++)
		{
			auto element = (i == j) ? 1.0 : 0.0;
			element -= prIeJepXJi * prIeJepXJ->at(j);
			pprIeJepXJipXJ->atiput(j, element / rIeJe);
		}
	}

	for (size_t i = 0; i < 3; i++)
	{
		auto& pprIeJepXIipEJ = pprIeJepXIpEJ->at(i);
		auto& prIeJepXIi = prIeJepXI->at(i);
		auto& prIeJeOipEJ = prIeJeOpEJ->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto element = 0.0 - prIeJeOipEJ->at(j) - prIeJepXIi * prIeJepEJ->at(j);
			pprIeJepXIipEJ->atiput(j, element / rIeJe);
		}
	}

	for (size_t i = 0; i < 4; i++)
	{
		auto& pprIeJepEIipEJ = pprIeJepEIpEJ->at(i);
		auto& prIeJepEIi = prIeJepEI->at(i);
		auto& mprIeJeOpEIiT = mprIeJeOpEIT->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto element = 0.0 - mprIeJeOpEIiT->dot(prIeJeOpEJT->at(j)) - prIeJepEIi * prIeJepEJ->at(j);
			pprIeJepEIipEJ->atiput(j, element / rIeJe);
		}
	}

	for (size_t i = 0; i < 3; i++)
	{
		auto& pprIeJepXJipEJ = pprIeJepXJpEJ->at(i);
		auto& prIeJepXJi = prIeJepXJ->at(i);
		auto& prIeJeOipEJ = prIeJeOpEJ->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto element = prIeJeOipEJ->at(j) - prIeJepXJi * prIeJepEJ->at(j);
			pprIeJepXJipEJ->atiput(j, element / rIeJe);
		}
	}

	for (size_t i = 0; i < 4; i++)
	{
		auto& pprIeJepEJipEJ = pprIeJepEJpEJ->at(i);
		auto& prIeJepEJi = prIeJepEJ->at(i);
		auto& pprIeJeOpEJipEJ = pprIeJeOpEJpEJ->at(i);
		auto& prIeJeOpEJiT = prIeJeOpEJT->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto element = prIeJeOpEJiT->dot(prIeJeOpEJT->at(j))
                           + pprIeJeOpEJipEJ->at(j)->dot(rIeJeO) - prIeJepEJi * prIeJepEJ->at(j);
			pprIeJepEJipEJ->atiput(j, element / rIeJe);
		}
	}
}

void MbD::DistIeqcJeqc::initialize()
{
	DistIeqcJec::initialize();
	prIeJepXJ = std::make_shared<FullRow<double>>(3);
	prIeJepEJ = std::make_shared<FullRow<double>>(4);
	pprIeJepXIpXJ = std::make_shared<FullMatrix<double>>(3, 3);
	pprIeJepEIpXJ = std::make_shared<FullMatrix<double>>(4, 3);
	pprIeJepXJpXJ = std::make_shared<FullMatrix<double>>(3, 3);
	pprIeJepXIpEJ = std::make_shared<FullMatrix<double>>(3, 4);
	pprIeJepEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	pprIeJepXJpEJ = std::make_shared<FullMatrix<double>>(3, 4);
	pprIeJepEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}

FMatDsptr MbD::DistIeqcJeqc::ppvaluepEIpEJ()
{
	return pprIeJepEIpEJ;
}

FMatDsptr MbD::DistIeqcJeqc::ppvaluepEIpXJ()
{
	return pprIeJepEIpXJ;
}

FMatDsptr MbD::DistIeqcJeqc::ppvaluepEJpEJ()
{
	return pprIeJepEJpEJ;
}

FMatDsptr MbD::DistIeqcJeqc::ppvaluepXIpEJ()
{
	return pprIeJepXIpEJ;
}

FMatDsptr MbD::DistIeqcJeqc::ppvaluepXIpXJ()
{
	return pprIeJepXIpXJ;
}

FMatDsptr MbD::DistIeqcJeqc::ppvaluepXJpEJ()
{
	return pprIeJepXJpEJ;
}

FMatDsptr MbD::DistIeqcJeqc::ppvaluepXJpXJ()
{
	return pprIeJepXJpXJ;
}

FRowDsptr MbD::DistIeqcJeqc::pvaluepEJ()
{
	return prIeJepEJ;
}

FRowDsptr MbD::DistIeqcJeqc::pvaluepXJ()
{
	return prIeJepXJ;
}
