/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "TranslationConstraintIqctJqc.h"
#include "DispCompIeqctJeqcKeqct.h"
#include "CREATE.h"

using namespace MbD;

TranslationConstraintIqctJqc::TranslationConstraintIqctJqc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi) :
	TranslationConstraintIqcJqc(frmi, frmj, axisi)
{
}

void TranslationConstraintIqctJqc::initriIeJeIe()
{
	riIeJeIe = CREATE<DispCompIeqctJeqcKeqct>::With(frmI, frmJ, frmI, axisI);
}

ConstraintType TranslationConstraintIqctJqc::type()
{
	return essential;
}

void TranslationConstraintIqctJqc::preVelIC()
{
	TranslationConstraintIJ::preVelIC();
	pGpt = std::static_pointer_cast<DispCompIeqctJeqcKeqct>(riIeJeIe)->pvaluept();
}

void TranslationConstraintIqctJqc::fillVelICError(FColDsptr col)
{
	col->atiminusNumber(iG, pGpt);
}

void TranslationConstraintIqctJqc::preAccIC()
{
	TranslationConstraintIJ::preAccIC();
	auto riIeJeIeqct = std::static_pointer_cast<DispCompIeqctJeqcKeqct>(riIeJeIe);
	ppGpXIpt = riIeJeIeqct->ppvaluepXIpt();
	ppGpEIpt = riIeJeIeqct->ppvaluepEIpt()->plusFullRow(riIeJeIeqct->ppvaluepEKpt());
	ppGpXJpt = riIeJeIeqct->ppvaluepXJpt();
	ppGpEJpt = riIeJeIeqct->ppvaluepEJpt();
	ppGptpt = riIeJeIeqct->ppvalueptpt();
}

void TranslationConstraintIqctJqc::fillAccICIterError(FColDsptr col)
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
