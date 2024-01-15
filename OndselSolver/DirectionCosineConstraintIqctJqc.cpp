/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DirectionCosineConstraintIqctJqc.h"
#include "DirectionCosineIeqctJeqc.h"
#include "CREATE.h"

using namespace MbD;

DirectionCosineConstraintIqctJqc::DirectionCosineConstraintIqctJqc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj) :
	DirectionCosineConstraintIqcJqc(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineConstraintIqctJqc::initaAijIeJe()
{
	aAijIeJe = CREATE<DirectionCosineIeqctJeqc>::With(frmI, frmJ, axisI, axisJ);
}

ConstraintType DirectionCosineConstraintIqctJqc::type()
{
	return essential;
}

void DirectionCosineConstraintIqctJqc::preVelIC()
{
	DirectionCosineConstraintIJ::preVelIC();
	pGpt = std::static_pointer_cast<DirectionCosineIeqctJeqc>(aAijIeJe)->pAijIeJept;
}

void DirectionCosineConstraintIqctJqc::fillVelICError(FColDsptr col)
{
	col->atiminusNumber(iG, pGpt);
}

void DirectionCosineConstraintIqctJqc::preAccIC()
{
	DirectionCosineConstraintIJ::preAccIC();
	ppGpEIpt = std::static_pointer_cast<DirectionCosineIeqctJeqc>(aAijIeJe)->ppAijIeJepEIpt;
	ppGpEJpt = std::static_pointer_cast<DirectionCosineIeqctJeqc>(aAijIeJe)->ppAijIeJepEJpt;
	ppGptpt = std::static_pointer_cast<DirectionCosineIeqctJeqc>(aAijIeJe)->ppAijIeJeptpt;
}

void DirectionCosineConstraintIqctJqc::fillAccICIterError(FColDsptr col)
{
	DirectionCosineConstraintIqcJqc::fillAccICIterError(col);
	auto efrmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto efrmJqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto qEdotI = efrmIqc->qEdot();
	auto qEdotJ = efrmJqc->qEdot();
	double sum = (ppGpEIpt->timesFullColumn(qEdotI)) * 2.0;
	sum += (ppGpEJpt->timesFullColumn(qEdotJ)) * 2.0;
	sum += ppGptpt;
	col->atiplusNumber(iG, sum);
}
