#include "DispCompIecJecKeqc.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::DispCompIecJecKeqc::DispCompIecJecKeqc()
{
}

MbD::DispCompIecJecKeqc::DispCompIecJecKeqc(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk) : DispCompIecJecKec(frmi, frmj, frmk, axisk)
{
}

void MbD::DispCompIecJecKeqc::initialize()
{
    priIeJeKepEK = std::make_shared<FullRow<double>>(4);
    ppriIeJeKepEKpEK = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::DispCompIecJecKeqc::initializeGlobally()
{
    ppAjOKepEKpEK = std::static_pointer_cast<EndFrameqc>(efrmK)->ppAjOepEpE(axisK);
}

void MbD::DispCompIecJecKeqc::calcPostDynCorrectorIteration()
{
	auto frmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto frmJqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto efrmKqc = std::static_pointer_cast<EndFrameqc>(efrmK);
	aAjOKe = efrmKqc->aAjOe(axisK);
	rIeJeO = frmJqc->rOeO->minusFullColumn(frmIqc->rOeO);
	riIeJeKe = aAjOKe->dot(rIeJeO);
	pAjOKepEKT = efrmKqc->pAjOepET(axisK);
	ppAjOKepEKpEK = efrmKqc->ppAjOepEpE(axisK);
	for (int i = 0; i < 4; i++)
	{
		priIeJeKepEK->at(i) = ((pAjOKepEKT->at(i))->dot(rIeJeO));
		auto& ppAjOKepEKipEK = ppAjOKepEKpEK->at(i);
		auto& ppriIeJeKepEKipEK = ppriIeJeKepEKpEK->at(i);
		ppriIeJeKepEKipEK->at(i) = ((ppAjOKepEKipEK->at(i))->dot(rIeJeO));
		for (int j = i + 1; j < 4; j++)
		{
			auto ppriIeJeKepEKipEKj = (ppAjOKepEKipEK->at(i))->dot(rIeJeO);
			ppriIeJeKepEKipEK->at(j) = ppriIeJeKepEKipEKj;
			ppriIeJeKepEKpEK->at(j)->at(i) = ppriIeJeKepEKipEKj;
		}
	}
}

FMatDsptr MbD::DispCompIecJecKeqc::ppvaluepEKpEK()
{
    return ppriIeJeKepEKpEK;
}
