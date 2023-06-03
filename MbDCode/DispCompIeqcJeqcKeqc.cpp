#include "DispCompIeqcJeqcKeqc.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::DispCompIeqcJeqcKeqc::DispCompIeqcJeqcKeqc()
{
}

MbD::DispCompIeqcJeqcKeqc::DispCompIeqcJeqcKeqc(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, size_t axisk) : DispCompIeqcJecKeqc(frmi, frmj, frmk, axisk)
{
}

void MbD::DispCompIeqcJeqcKeqc::initialize()
{
	DispCompIeqcJecKeqc::initialize();
	priIeJeKepXJ = std::make_shared<FullRow<double>>(3);
	priIeJeKepEJ = std::make_shared<FullRow<double>>(4);
	ppriIeJeKepEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppriIeJeKepXJpEK = std::make_shared<FullMatrix<double>>(3, 4);
	ppriIeJeKepEJpEK = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::DispCompIeqcJeqcKeqc::calcPostDynCorrectorIteration()
{
	DispCompIeqcJecKeqc::calcPostDynCorrectorIteration();
	auto frmJqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto prIeJeOpEJT = frmJqc->prOeOpE->transpose();
	auto pprIeJeOpEJpEJ = frmJqc->pprOeOpEpE;
	for (size_t i = 0; i < 3; i++)
	{
		priIeJeKepXJ->at(i) = (aAjOKe->at(i));
	}
	for (size_t i = 0; i < 4; i++)
	{
		priIeJeKepEJ->at(i) = (aAjOKe->dot(prIeJeOpEJT->at(i)));
	}
	for (size_t i = 0; i < 3; i++)
	{
		auto& ppriIeJeKepXJipEK = ppriIeJeKepXJpEK->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			ppriIeJeKepXJipEK->at(j) = (pAjOKepEKT->at(j)->at(i));
		}
	}
	for (size_t i = 0; i < 4; i++)
	{
		auto& pprIeJeOpEJipEJ = pprIeJeOpEJpEJ->at(i);
		auto& ppriIeJeKepEJipEJ = ppriIeJeKepEJpEJ->at(i);
		ppriIeJeKepEJipEJ->at(i) = (aAjOKe->dot(pprIeJeOpEJipEJ->at(i)));
		for (size_t j = 0; j < 4; j++)
		{
			auto ppriIeJeKepEJipEJj = (aAjOKe->dot(pprIeJeOpEJipEJ->at(j)));
			ppriIeJeKepEJipEJ->at(j) = ppriIeJeKepEJipEJj;
			ppriIeJeKepEJpEJ->at(j)->at(i) = ppriIeJeKepEJipEJj;
		}
	}
	for (size_t i = 0; i < 4; i++)
	{
		auto& prIeJeOpEJTi = prIeJeOpEJT->at(i);
		auto& ppriIeJeKepEJipEK = ppriIeJeKepEJpEK->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			ppriIeJeKepEJipEK->at(j) = (pAjOKepEKT->at(j)->dot(prIeJeOpEJTi));
		}
	}
}
