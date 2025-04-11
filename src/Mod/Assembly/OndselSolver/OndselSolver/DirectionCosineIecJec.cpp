/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <memory>

#include "DirectionCosineIecJec.h"
#include "EndFramec.h"

using namespace MbD;

DirectionCosineIecJec::DirectionCosineIecJec()
= default;

DirectionCosineIecJec::DirectionCosineIecJec(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj) :
	KinematicIeJe(frmi, frmj), axisI(axisi), axisJ(axisj)
{

}

void DirectionCosineIecJec::calcPostDynCorrectorIteration()
{
	aAjOIe = frmI->aAjOe(axisI);
	aAjOJe = frmJ->aAjOe(axisJ);
	aAijIeJe = aAjOIe->dot(aAjOJe);
}

double MbD::DirectionCosineIecJec::value()
{
	return aAijIeJe;
}
