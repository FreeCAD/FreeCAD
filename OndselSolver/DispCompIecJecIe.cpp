/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DispCompIecJecIe.h"
#include "EndFramec.h"

using namespace MbD;

MbD::DispCompIecJecIe::DispCompIecJecIe()
{
}

MbD::DispCompIecJecIe::DispCompIecJecIe(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis) : KinematicIeJe(frmi, frmj), axis(axis)
{
}

void MbD::DispCompIecJecIe::calc_value()
{
	aAjOIe = frmI->aAjOe(axis);
	rIeJeO = frmJ->rOeO->minusFullColumn(frmI->rOeO);
	riIeJeIe = aAjOIe->dot(rIeJeO);
}

void MbD::DispCompIecJecIe::calcPostDynCorrectorIteration()
{
	calc_value();
}

double MbD::DispCompIecJecIe::value()
{
	return riIeJeIe;
}
