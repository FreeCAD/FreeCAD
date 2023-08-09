/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "GearConstraintIJ.h"

using namespace MbD;

MbD::GearConstraintIJ::GearConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj) : ConstraintIJ(frmi, frmj)
{
}

void MbD::GearConstraintIJ::calcPostDynCorrectorIteration()
{
	aG = orbitJeIe->value() + (this->ratio() * orbitIeJe->value()) - aConstant;
}

void MbD::GearConstraintIJ::initialize()
{
	ConstraintIJ::initialize();
	this->initorbitsIJ();
}

void MbD::GearConstraintIJ::initializeGlobally()
{
	orbitIeJe->initializeGlobally();
	orbitJeIe->initializeGlobally();
}

void MbD::GearConstraintIJ::initializeLocally()
{
	orbitIeJe->initializeLocally();
	orbitJeIe->initializeLocally();
}

void MbD::GearConstraintIJ::initorbitsIJ()
{
	assert(false);
}

void MbD::GearConstraintIJ::postInput()
{
	orbitIeJe->postInput();
	orbitJeIe->postInput();
	aConstant = orbitJeIe->value() + (this->ratio() * orbitIeJe->value());
	ConstraintIJ::postInput();
}

void MbD::GearConstraintIJ::postPosICIteration()
{
	orbitIeJe->postPosICIteration();
	orbitJeIe->postPosICIteration();
	ConstraintIJ::postPosICIteration();
}

void MbD::GearConstraintIJ::preAccIC()
{
	orbitIeJe->preAccIC();
	orbitJeIe->preAccIC();
	ConstraintIJ::preAccIC();
}

void MbD::GearConstraintIJ::prePosIC()
{
	orbitIeJe->prePosIC();
	orbitJeIe->prePosIC();
	ConstraintIJ::prePosIC();
}

void MbD::GearConstraintIJ::preVelIC()
{
	orbitIeJe->preVelIC();
	orbitJeIe->preVelIC();
	ConstraintIJ::preVelIC();
}

double MbD::GearConstraintIJ::ratio()
{
	return radiusI / radiusJ;
}

void MbD::GearConstraintIJ::simUpdateAll()
{
	orbitIeJe->simUpdateAll();
	orbitJeIe->simUpdateAll();
	ConstraintIJ::simUpdateAll();
}
