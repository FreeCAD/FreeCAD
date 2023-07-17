#include "AngleZIeqcJec.h"
#include "CREATE.h"

using namespace MbD;

MbD::AngleZIeqcJec::AngleZIeqcJec()
{
}

MbD::AngleZIeqcJec::AngleZIeqcJec(EndFrmcptr frmi, EndFrmcptr frmj) : AngleZIecJec(frmi, frmj)
{
}

void MbD::AngleZIeqcJec::calcPostDynCorrectorIteration()
{
	AngleZIecJec::calcPostDynCorrectorIteration();
	auto pcthezpEI = aA00IeJe->pvaluepEI();
	auto psthezpEI = aA10IeJe->pvaluepEI();
	auto ppcthezpEIpEI = aA00IeJe->ppvaluepEIpEI();
	auto ppsthezpEIpEI = aA10IeJe->ppvaluepEIpEI();
	for (int i = 0; i < 4; i++)
	{
		pthezpEI->atiput(i, (psthezpEI->at(i)) * cosOverSSq - ((pcthezpEI->at(i)) * sinOverSSq));
	}
	for (int i = 0; i < 4; i++)
	{
		auto ppthezpEIpEIi = ppthezpEIpEI->at(i);
		auto ppcthezpEIpEIi = ppcthezpEIpEI->at(i);
		auto ppsthezpEIpEIi = ppsthezpEIpEI->at(i);
		auto pcthezpEIi = pcthezpEI->at(i);
		auto psthezpEIi = psthezpEI->at(i);
		auto term1 = (pcthezpEIi * pcthezpEIi - (psthezpEIi * psthezpEIi)) * twoCosSinOverSSqSq;
		auto term2 = ppsthezpEIpEIi->at(i) * cosOverSSq - (ppcthezpEIpEIi->at(i) * sinOverSSq);
		auto term3 = (psthezpEIi * pcthezpEIi + (pcthezpEIi * psthezpEIi)) * dSqOverSSqSq;
		ppthezpEIpEIi->atiput(i, term1 + term2 + term3);
		for (int j = i + 1; j < 4; j++)
		{
			auto pcthezpEIj = pcthezpEI->at(j);
			auto psthezpEIj = psthezpEI->at(j);
			auto term1 = (pcthezpEIi * pcthezpEIj - (psthezpEIi * psthezpEIj)) * twoCosSinOverSSqSq;
			auto term2 = ppsthezpEIpEIi->at(j) * cosOverSSq - (ppcthezpEIpEIi->at(j) * sinOverSSq);
			auto term3 = (psthezpEIi * pcthezpEIj + (pcthezpEIi * psthezpEIj)) * dSqOverSSqSq;
			auto ppthezpEIpEIij = term1 + term2 + term3;
			ppthezpEIpEIi->atiput(j, ppthezpEIpEIij);
			ppthezpEIpEI->atijput(j, i, ppthezpEIpEIij);
		}
	}
}

void MbD::AngleZIeqcJec::init_aAijIeJe()
{
	aA00IeJe = CREATE<DirectionCosineIeqcJec>::With(frmI, frmJ, 0, 0);
	aA10IeJe = CREATE<DirectionCosineIeqcJec>::With(frmI, frmJ, 1, 0);
}

void MbD::AngleZIeqcJec::initialize()
{
	AngleZIecJec::initialize();
	pthezpEI = std::make_shared<FullRow<double>>(4);
	ppthezpEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

FMatDsptr MbD::AngleZIeqcJec::ppvaluepEIpEI()
{
	return ppthezpEIpEI;
}

FRowDsptr MbD::AngleZIeqcJec::pvaluepEI()
{
	return pthezpEI;
}
