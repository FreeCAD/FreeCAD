/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DirectionCosineIeqctJeqc.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"

using namespace MbD;

DirectionCosineIeqctJeqc::DirectionCosineIeqctJeqc()
{
}

DirectionCosineIeqctJeqc::DirectionCosineIeqctJeqc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj) :
	DirectionCosineIeqcJeqc(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineIeqctJeqc::initialize()
{
	DirectionCosineIeqcJeqc::initialize();
	ppAijIeJepEIpt = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEJpt = std::make_shared<FullRow<double>>(4);
}

void DirectionCosineIeqctJeqc::initializeGlobally()
{
	ppAjOJepEJpEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->ppAjOepEpE(axisJ);
}

FRowDsptr MbD::DirectionCosineIeqctJeqc::ppvaluepEIpt()
{
	return ppAijIeJepEIpt;
}

FRowDsptr MbD::DirectionCosineIeqctJeqc::ppvaluepEJpt()
{
	return ppAijIeJepEJpt;
}

double MbD::DirectionCosineIeqctJeqc::ppvalueptpt()
{
	return ppAijIeJeptpt;
}

void DirectionCosineIeqctJeqc::calcPostDynCorrectorIteration()
{
	//"ppAjOIepEIpEI is not longer constant and must be set before any calculation."

	ppAjOIepEIpEI = std::static_pointer_cast<EndFrameqc>(frmI)->ppAjOepEpE(axisI);
	DirectionCosineIeqcJeqc::calcPostDynCorrectorIteration();
}

void DirectionCosineIeqctJeqc::preVelIC()
{
	Item::preVelIC();
	auto pAjOIept = std::static_pointer_cast<EndFrameqct>(frmI)->pAjOept(axisI);
	pAijIeJept = pAjOIept->dot(aAjOJe);
}

double DirectionCosineIeqctJeqc::pvaluept()
{
	return pAijIeJept;
}

void DirectionCosineIeqctJeqc::preAccIC()
{
	//| ppAjOIepEITpt ppAjOIeptpt ppAjOIepEITpti pAjOIept |
	Item::preAccIC();
	auto pAjOIept = std::static_pointer_cast<EndFrameqct>(frmI)->pAjOept(axisI);
	auto ppAjOIepEITpt = std::static_pointer_cast<EndFrameqct>(frmI)->ppAjOepETpt(axisI);
	auto ppAjOIeptpt = std::static_pointer_cast<EndFrameqct>(frmI)->ppAjOeptpt(axisI);
	for (size_t i = 0; i < 4; i++)
	{
		auto& ppAjOIepEITpti = ppAjOIepEITpt->at(i);
		ppAijIeJepEIpt->atiput(i, ppAjOIepEITpti->dot(aAjOJe));
	}
	for (size_t i = 0; i < 4; i++)
	{
		ppAijIeJepEJpt->atiput(i, pAjOIept->dot(pAjOJepEJT->at(i)));
	}
	ppAijIeJeptpt = ppAjOIeptpt->dot(aAjOJe);
}
