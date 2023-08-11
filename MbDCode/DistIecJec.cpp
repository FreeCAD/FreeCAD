/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DistIecJec.h"
#include "EndFramec.h"

using namespace MbD;

MbD::DistIecJec::DistIecJec()
{
}

MbD::DistIecJec::DistIecJec(EndFrmsptr frmi, EndFrmsptr frmj) : KinematicIeJe(frmi, frmj)
{
}

void MbD::DistIecJec::calcPostDynCorrectorIteration()
{
	rIeJeO = frmJ->rOeO->minusFullColumn(frmI->rOeO);
	rIeJe = rIeJeO->length();
	this->calcPrivate();
}

void MbD::DistIecJec::calcPrivate()
{
	if (rIeJe == 0.0) return;
	uIeJeO = rIeJeO->times(1.0 / rIeJe);
	muIeJeO = uIeJeO->negated();
}

double MbD::DistIecJec::value()
{
	return rIeJe;
}
