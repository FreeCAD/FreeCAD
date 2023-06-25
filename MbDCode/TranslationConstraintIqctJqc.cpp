#include "TranslationConstraintIqctJqc.h"
#include "DispCompIeqctJeqcKeqct.h"
#include "CREATE.h"

using namespace MbD;

TranslationConstraintIqctJqc::TranslationConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	TranslationConstraintIqcJqc(frmi, frmj, axisi)
{
}

void MbD::TranslationConstraintIqctJqc::initriIeJeIe()
{
	riIeJeIe = CREATE<DispCompIeqctJeqcKeqct>::With(frmI, frmJ, frmI, axisI);
}

MbD::ConstraintType MbD::TranslationConstraintIqctJqc::type()
{
	return MbD::essential;
}

void MbD::TranslationConstraintIqctJqc::preVelIC()
{
	TranslationConstraintIJ::preVelIC();
	pGpt = std::static_pointer_cast<DispCompIeqctJeqcKeqct>(riIeJeIe)->pvaluept();
}

void MbD::TranslationConstraintIqctJqc::fillVelICError(FColDsptr col)
{
	col->atiminusNumber(iG, pGpt);
}

void MbD::TranslationConstraintIqctJqc::preAccIC()
{
	TranslationConstraintIJ::preAccIC();
	auto riIeJeIeqct = std::static_pointer_cast<DispCompIeqctJeqcKeqct>(riIeJeIe);
	ppGpXIpt = riIeJeIeqct->ppvaluepXIpt();
	ppGpEIpt = riIeJeIeqct->ppvaluepEIpt()->plusFullRow(riIeJeIeqct->ppvaluepEKpt());
	ppGpXJpt = riIeJeIeqct->ppvaluepXJpt();
	ppGpEJpt = riIeJeIeqct->ppvaluepEJpt();
	ppGptpt = riIeJeIeqct->ppvalueptpt();
}

void MbD::TranslationConstraintIqctJqc::fillAccICIterError(FColDsptr col)
{
	TranslationConstraintIqcJqc::fillAccICIterError(col);
	auto efrmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto efrmJqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto qXdotI = efrmIqc->qXdot();
	auto qEdotI = efrmIqc->qEdot();
	auto qXdotJ = efrmJqc->qXdot();
	auto qEdotJ = efrmJqc->qEdot();
	double sum = 2.0 * ppGpXIpt->timesFullColumn(qXdotI);
	sum += 2.0 * ppGpEIpt->timesFullColumn(qEdotI);
	sum += 2.0 * ppGpXJpt->timesFullColumn(qXdotJ);
	sum += 2.0 * ppGpEJpt->timesFullColumn(qEdotJ);
	sum += ppGptpt;
	col->atiplusNumber(iG, sum);
}
