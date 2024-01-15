/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DirectionCosineIeqcJec.h"
#include "EndFrameqc.h"

using namespace MbD;

DirectionCosineIeqcJec::DirectionCosineIeqcJec()
{
}

DirectionCosineIeqcJec::DirectionCosineIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj) :
	DirectionCosineIecJec(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineIeqcJec::initialize()
{
	DirectionCosineIecJec::initialize();
	pAijIeJepEI = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

void DirectionCosineIeqcJec::initializeGlobally()
{
	ppAjOIepEIpEI = std::static_pointer_cast<EndFrameqc>(frmI)->ppAjOepEpE(axisI);
}

FMatDsptr MbD::DirectionCosineIeqcJec::ppvaluepEIpEI()
{
	return ppAijIeJepEIpEI;
}

FRowDsptr MbD::DirectionCosineIeqcJec::pvaluepEI()
{
	return pAijIeJepEI;
}

void DirectionCosineIeqcJec::calcPostDynCorrectorIteration()
{
	DirectionCosineIecJec::calcPostDynCorrectorIteration();
	pAjOIepEIT = std::static_pointer_cast<EndFrameqc>(frmI)->pAjOepET(axisI);
	for (size_t i = 0; i < 4; i++)
	{
		pAijIeJepEI->at(i) = pAjOIepEIT->at(i)->dot(aAjOJe);
	}
	for (size_t i = 0; i < 4; i++)
	{
		auto& ppAijIeJepEIipEI = ppAijIeJepEIpEI->at(i);
		auto& ppAjOIepEIipEI = ppAjOIepEIpEI->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			ppAijIeJepEIipEI->at(j) = ppAjOIepEIipEI->at(j)->dot(aAjOJe);
		}
	}	
	ppAijIeJepEIpEI->symLowerWithUpper();
}
