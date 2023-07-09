#include "OrbitAnglezIeqcJeqc.h"

using namespace MbD;

void MbD::OrbitAnglezIeqcJeqc::calc_ppthezpEIpEJ()
{
	auto pxpEI = xIeJeIe->pvaluepEI();
	auto pypEI = yIeJeIe->pvaluepEI();
	auto pxpEJ = xIeJeIe->pvaluepEJ();
	auto pypEJ = yIeJeIe->pvaluepEJ();
	auto ppxpEIpEJ = xIeJeIe->ppvaluepEIpEJ();
	auto ppypEIpEJ = yIeJeIe->ppvaluepEIpEJ();
	for (int i = 0; i < 4; i++)
	{
		auto ppthezpEIpEJi = ppthezpEIpEJ->at(i);
		auto ppxpEIpEJi = ppxpEIpEJ->at(i);
		auto ppypEIpEJi = ppypEIpEJ->at(i);
		auto pxpEIi = pxpEI->at(i);
		auto pypEIi = pypEI->at(i);
		for (int j = 0; j < 4; j++)
		{
			auto pxpEJj = pxpEJ->at(j);
			auto pypEJj = pypEJ->at(j);
			auto term1 = (pxpEIi * pxpEJj - (pypEIi * pypEJj)) * twoCosSinOverSSqSq;
			auto term2 = ppypEIpEJi->at(j) * cosOverSSq - (ppxpEIpEJi->at(j) * sinOverSSq);
			auto term3 = (pypEIi * pxpEJj + (pxpEIi * pypEJj)) * dSqOverSSqSq;
			ppthezpEIpEJi->atiput(j, term1 + term2 + term3);
		}
	}
}

void MbD::OrbitAnglezIeqcJeqc::calc_ppthezpEIpXJ()
{
	//ppxpEIpXJ = 0
	//ppypEIpXJ = 0

	auto pxpEI = xIeJeIe->pvaluepEI();
	auto pypEI = yIeJeIe->pvaluepEI();
	auto pxpXJ = xIeJeIe->pvaluepXJ();
	auto pypXJ = yIeJeIe->pvaluepXJ();
	for (int i = 0; i < 4; i++)
	{
		auto ppthezpEIpXJi = ppthezpEIpXJ->at(i);
		auto pxpEIi = pxpEI->at(i);
		auto pypEIi = pypEI->at(i);
		for (int j = 0; j < 3; j++)
		{
			auto pxpXJj = pxpXJ->at(j);
			auto pypXJj = pypXJ->at(j);
			auto term1 = (pxpEIi * pxpXJj - (pypEIi * pypXJj)) * twoCosSinOverSSqSq;
			auto term3 = (pypEIi * pxpXJj + (pxpEIi * pypXJj)) * dSqOverSSqSq;
			ppthezpEIpXJi->atiput(j, term1 + term3);
		}
	}
}

void MbD::OrbitAnglezIeqcJeqc::calc_ppthezpEJpEJ()
{
	auto pxpEJ = xIeJeIe->pvaluepEJ();
	auto pypEJ = yIeJeIe->pvaluepEJ();
	auto ppxpEJpEJ = xIeJeIe->ppvaluepEJpEJ();
	auto ppypEJpEJ = yIeJeIe->ppvaluepEJpEJ();
	for (int i = 0; i < 4; i++)
	{
		auto ppthezpEJpEJi = ppthezpEJpEJ->at(i);
		auto ppxpEJpEJi = ppxpEJpEJ->at(i);
		auto ppypEJpEJi = ppypEJpEJ->at(i);
		auto pxpEJi = pxpEJ->at(i);
		auto pypEJi = pypEJ->at(i);
		for (int j = i; j < 4; j++)
		{
			auto pxpEJj = pxpEJ->at(j);
			auto pypEJj = pypEJ->at(j);
			auto term1 = (pxpEJi * pxpEJj - (pypEJi * pypEJj)) * twoCosSinOverSSqSq;
			auto term2 = ppypEJpEJi->at(j) * cosOverSSq - (ppxpEJpEJi->at(j) * sinOverSSq);
			auto term3 = (pypEJi * pxpEJj + (pxpEJi * pypEJj)) * dSqOverSSqSq;
			auto ppthezpEJpEJij = term1 + term2 + term3;
			ppthezpEJpEJi->atiput(j, ppthezpEJpEJij);
			if (i < j) ppthezpEJpEJ->atijput(j, i, ppthezpEJpEJij);
		}
	}
}

void MbD::OrbitAnglezIeqcJeqc::calc_ppthezpXIpEJ()
{
	//ppxpXIpEJ = 0
	//ppypXIpEJ = 0

	auto pxpXI = xIeJeIe->pvaluepXI();
	auto pypXI = yIeJeIe->pvaluepXI();
	auto pxpEJ = xIeJeIe->pvaluepEJ();
	auto pypEJ = yIeJeIe->pvaluepEJ();
	for (int i = 0; i < 3; i++)
	{
		auto ppthezpXIpEJi = ppthezpXIpEJ->at(i);
		auto pxpXIi = pxpXI->at(i);
		auto pypXIi = pypXI->at(i);
		for (int j = 0; j < 4; j++)
		{
			auto pxpEJj = pxpEJ->at(j);
			auto pypEJj = pypEJ->at(j);
			auto term1 = (pxpXIi * pxpEJj - (pypXIi * pypEJj)) * twoCosSinOverSSqSq;
			auto term3 = (pypXIi * pxpEJj + (pxpXIi * pypEJj)) * dSqOverSSqSq;
			ppthezpXIpEJi->atiput(j, term1 + term3);
		}
	}
}

void MbD::OrbitAnglezIeqcJeqc::calc_ppthezpXIpXJ()
{
	//ppxpXIpXJ = 0
	//ppypXIpXJ = 0

	auto pxpXI = xIeJeIe->pvaluepXI();
	auto pypXI = yIeJeIe->pvaluepXI();
	auto pxpXJ = xIeJeIe->pvaluepXJ();
	auto pypXJ = yIeJeIe->pvaluepXJ();
	for (int i = 0; i < 3; i++)
	{
		auto ppthezpXIpXJi = ppthezpXIpXJ->at(i);
		auto pxpXIi = pxpXI->at(i);
		auto pypXIi = pypXI->at(i);
		for (int j = 0; j < 3; j++)
		{
			auto pxpXJj = pxpXJ->at(j);
			auto pypXJj = pypXJ->at(j);
			auto term1 = (pxpXIi * pxpXJj - (pypXIi * pypXJj)) * twoCosSinOverSSqSq;
			auto term3 = (pypXIi * pxpXJj + (pxpXIi * pypXJj)) * dSqOverSSqSq;
			ppthezpXIpXJi->atiput(j, term1 + term3);
		}
	}
}

void MbD::OrbitAnglezIeqcJeqc::calc_ppthezpXJpEJ()
{
	//ppxpXJpEJ = 0
	//ppypXJpEJ = 0

	auto pxpXJ = xIeJeIe->pvaluepXJ();
	auto pypXJ = yIeJeIe->pvaluepXJ();
	auto pxpEJ = xIeJeIe->pvaluepEJ();
	auto pypEJ = yIeJeIe->pvaluepEJ();
	for (int i = 0; i < 3; i++)
	{
		auto ppthezpXJpEJi = ppthezpXJpEJ->at(i);
		auto pxpXJi = pxpXJ->at(i);
		auto pypXJi = pypXJ->at(i);
		for (int j = 0; j < 4; j++)
		{
			auto pxpEJj = pxpEJ->at(j);
			auto pypEJj = pypEJ->at(j);
			auto term1 = (pxpXJi * pxpEJj - (pypXJi * pypEJj)) * twoCosSinOverSSqSq;
			auto term3 = (pypXJi * pxpEJj + (pxpXJi * pypEJj)) * dSqOverSSqSq;
			ppthezpXJpEJi->atiput(j, term1 + term3);
		}
	}
}

void MbD::OrbitAnglezIeqcJeqc::calc_ppthezpXJpXJ()
{
	//ppxpXJpXJ = 0
	//ppypXJpXJ = 0

	auto pxpXJ = xIeJeIe->pvaluepXJ();
	auto pypXJ = yIeJeIe->pvaluepXJ();
	for (int i = 0; i < 3; i++)
	{
		auto ppthezpXJpXJi = ppthezpXJpXJ->at(i);
		auto pxpXJi = pxpXJ->at(i);
		auto pypXJi = pypXJ->at(i);
		for (int j = 0; j < 3; j++)
		{
			auto pxpXJj = pxpXJ->at(j);
			auto pypXJj = pypXJ->at(j);
			auto term1 = (pxpXJi * pxpXJj - (pypXJi * pypXJj)) * twoCosSinOverSSqSq;
			auto term3 = (pypXJi * pxpXJj + (pxpXJi * pypXJj)) * dSqOverSSqSq;
			ppthezpXJpXJi->atiput(j, term1 + term3);
		}
	}
}

void MbD::OrbitAnglezIeqcJeqc::calc_pthezpEJ()
{
	auto pxpEJ = xIeJeIe->pvaluepEJ();
	auto pypEJ = yIeJeIe->pvaluepEJ();
	for (int i = 0; i < 4; i++)
	{
		pthezpEJ->atiput(i, pypEJ->at(i) * cosOverSSq - (pxpEJ->at(i) * sinOverSSq));
	}
}

void MbD::OrbitAnglezIeqcJeqc::calc_pthezpXJ()
{
	auto pxpXJ = xIeJeIe->pvaluepXJ();
	auto pypXJ = yIeJeIe->pvaluepXJ();
	for (int i = 0; i < 3; i++)
	{
		pthezpXJ->atiput(i, pypXJ->at(i) * cosOverSSq - (pxpXJ->at(i) * sinOverSSq));
	}
}

void MbD::OrbitAnglezIeqcJeqc::calcPostDynCorrectorIteration()
{
	OrbitAnglezIeqcJec::calcPostDynCorrectorIteration();
	this->calc_pthezpXJ();
	this->calc_pthezpEJ();
	this->calc_ppthezpXIpXJ();
	this->calc_ppthezpXIpEJ();
	this->calc_ppthezpEIpXJ();
	this->calc_ppthezpEIpEJ();
	this->calc_ppthezpXJpXJ();
	this->calc_ppthezpXJpEJ();
	this->calc_ppthezpEJpEJ();
}

void MbD::OrbitAnglezIeqcJeqc::initialize()
{
	OrbitAnglezIeqcJec::initialize();
	pthezpXJ = std::make_shared<FullRow<double>>(3);
	pthezpEJ = std::make_shared<FullRow<double>>(4);
	ppthezpXIpXJ = std::make_shared<FullMatrix<double>>(3, 3);
	ppthezpXIpEJ = std::make_shared<FullMatrix<double>>(3, 4);
	ppthezpEIpXJ = std::make_shared<FullMatrix<double>>(4, 3);
	ppthezpEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppthezpXJpXJ = std::make_shared<FullMatrix<double>>(3, 3);
	ppthezpXJpEJ = std::make_shared<FullMatrix<double>>(3, 4);
	ppthezpEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}

FMatDsptr MbD::OrbitAnglezIeqcJeqc::ppvaluepEIpEJ()
{
	return ppthezpEIpEJ;
}

FMatDsptr MbD::OrbitAnglezIeqcJeqc::ppvaluepEIpXJ()
{
	return ppthezpEIpXJ;
}

FMatDsptr MbD::OrbitAnglezIeqcJeqc::ppvaluepEJpEJ()
{
	return ppthezpEJpEJ;
}

FMatDsptr MbD::OrbitAnglezIeqcJeqc::ppvaluepXIpEJ()
{
	return ppthezpXIpEJ;
}

FMatDsptr MbD::OrbitAnglezIeqcJeqc::ppvaluepXIpXJ()
{
	return ppthezpXIpXJ;
}

FMatDsptr MbD::OrbitAnglezIeqcJeqc::ppvaluepXJpEJ()
{
	return ppthezpXJpEJ;
}

FMatDsptr MbD::OrbitAnglezIeqcJeqc::ppvaluepXJpXJ()
{
	return ppthezpXJpXJ;
}

FRowDsptr MbD::OrbitAnglezIeqcJeqc::pvaluepEJ()
{
	return pthezpEJ;
}

FRowDsptr MbD::OrbitAnglezIeqcJeqc::pvaluepXJ()
{
	return pthezpXJ;
}
