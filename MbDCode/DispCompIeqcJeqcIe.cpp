#include "DispCompIeqcJeqcIe.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::DispCompIeqcJeqcIe::DispCompIeqcJeqcIe()
{
}

MbD::DispCompIeqcJeqcIe::DispCompIeqcJeqcIe(EndFrmcptr frmi, EndFrmcptr frmj, int axis) : DispCompIeqcJecIe(frmi, frmj, axis)
{
}

void MbD::DispCompIeqcJeqcIe::calc_ppvaluepEIpEJ()
{
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto& prIeJeOpEJ = frmJeqc->prOeOpE;
	ppriIeJeIepEIpEJ = pAjOIepEIT->timesFullMatrix(prIeJeOpEJ);
}

void MbD::DispCompIeqcJeqcIe::calc_ppvaluepEIpXJ()
{
	ppriIeJeIepEIpXJ = pAjOIepEIT;
}

void MbD::DispCompIeqcJeqcIe::calc_ppvaluepEJpEJ()
{
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto pprIeJeOpEJpEJ = frmJeqc->pprOeOpEpE;
	for (int i = 0; i < 4; i++)
	{
		auto pprIeJeOpEJipEJ = pprIeJeOpEJpEJ->at(i);
		auto ppriIeJeIepEJipEJ = ppriIeJeIepEJpEJ->at(i);
		for (int j = i; j < 4; j++)
		{
			auto term1 = aAjOIe->dot(pprIeJeOpEJipEJ->at(j));
			ppriIeJeIepEJipEJ->atiput(j, term1);
		}
	}
	ppriIeJeIepEJpEJ->symLowerWithUpper();
}

void MbD::DispCompIeqcJeqcIe::calc_pvaluepEJ()
{
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto prIeJeOpEJT = frmJeqc->prOeOpE->transpose();
	for (int i = 0; i < 4; i++)
	{
		priIeJeIepEJ->atiput(i, aAjOIe->dot(prIeJeOpEJT->at(i)));
	}
}

void MbD::DispCompIeqcJeqcIe::calc_pvaluepXJ()
{
	for (int i = 0; i < 3; i++)
	{
		priIeJeIepXJ->atiput(i, aAjOIe->at(i));
	}
}

void MbD::DispCompIeqcJeqcIe::calcPostDynCorrectorIteration()
{
	//Must maintain order of calc_xxx.
	DispCompIeqcJecIe::calcPostDynCorrectorIteration();
	calc_pvaluepXJ();
	calc_pvaluepEJ();
	calc_ppvaluepEIpXJ();
	calc_ppvaluepEIpEJ();
	calc_ppvaluepEJpEJ();

}

void MbD::DispCompIeqcJeqcIe::initialize()
{
	DispCompIeqcJecIe::initialize();
	priIeJeIepXJ = std::make_shared<FullRow<double>>(3);
	priIeJeIepEJ = std::make_shared<FullRow<double>>(4);
	ppriIeJeIepEIpXJ = std::make_shared<FullMatrix<double>>(4, 3);
	ppriIeJeIepEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppriIeJeIepEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}

FMatDsptr MbD::DispCompIeqcJeqcIe::ppvaluepEIpEJ()
{
	return ppriIeJeIepEIpEJ;
}

FMatDsptr MbD::DispCompIeqcJeqcIe::ppvaluepEIpXJ()
{
	return ppriIeJeIepEIpXJ;
}

FMatDsptr MbD::DispCompIeqcJeqcIe::ppvaluepEJpEJ()
{
	return ppriIeJeIepEJpEJ;
}

FRowDsptr MbD::DispCompIeqcJeqcIe::pvaluepEJ()
{
	return priIeJeIepEJ;
}

FRowDsptr MbD::DispCompIeqcJeqcIe::pvaluepXJ()
{
	return priIeJeIepXJ;
}
