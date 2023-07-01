#include "DispCompIeqctJeqcKeqct.h"
#include "EndFrameqct.h"

using namespace MbD;

DispCompIeqctJeqcKeqct::DispCompIeqctJeqcKeqct()
{
}

DispCompIeqctJeqcKeqct::DispCompIeqctJeqcKeqct(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk) : DispCompIeqcJeqcKeqct(frmi, frmj, frmk, axisk)
{
}

void DispCompIeqctJeqcKeqct::preVelIC()
{
	DispCompIeqcJeqcKeqct::preVelIC();
	auto& mprIeJeOpt = std::static_pointer_cast<EndFrameqct>(frmI)->prOeOpt;
	priIeJeKept -= aAjOKe->dot(mprIeJeOpt);
}

void DispCompIeqctJeqcKeqct::preAccIC()
{
	DispCompIeqcJeqcKeqct::preAccIC();
	auto pAjOKept = std::static_pointer_cast<EndFrameqct>(efrmK)->pAjOept(axisK);
	auto efrmIqct = std::static_pointer_cast<EndFrameqct>(frmI);
	auto& mprIeJeOpt = efrmIqct->prOeOpt;
	auto mpprIeJeOpEITpt = efrmIqct->pprOeOpEpt->transpose();
	auto& mpprIeJeOptpt = efrmIqct->pprOeOptpt;
	for (int i = 0; i < 4; i++)
	{
		ppriIeJeKepEIpt->atiminusNumber(i, aAjOKe->dot(mpprIeJeOpEITpt->at(i)));
		ppriIeJeKepEKpt->atiminusNumber(i, pAjOKepEKT->at(i)->dot(mprIeJeOpt));
	}
	ppriIeJeKeptpt +=  -(2.0 * pAjOKept->dot(mprIeJeOpt)) - aAjOKe->dot(mpprIeJeOptpt);
}
