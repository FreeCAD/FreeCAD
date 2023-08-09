/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DistancexyConstraintIJ.h"

using namespace MbD;

MbD::DistancexyConstraintIJ::DistancexyConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj) : ConstraintIJ(frmi, frmj)
{
}

void MbD::DistancexyConstraintIJ::calcPostDynCorrectorIteration()
{
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	aG = x * x + (y * y) - (aConstant * aConstant);
}

void MbD::DistancexyConstraintIJ::init_xyIeJeIe()
{
	assert(false);
}

void MbD::DistancexyConstraintIJ::initialize()
{
	ConstraintIJ::initialize();
	this->init_xyIeJeIe();
}

void MbD::DistancexyConstraintIJ::initializeGlobally()
{
	xIeJeIe->initializeGlobally();
	yIeJeIe->initializeGlobally();
}

void MbD::DistancexyConstraintIJ::initializeLocally()
{
	xIeJeIe->initializeLocally();
	yIeJeIe->initializeLocally();
}

void MbD::DistancexyConstraintIJ::postInput()
{
	xIeJeIe->postInput();
	yIeJeIe->postInput();
	ConstraintIJ::postInput();
}

void MbD::DistancexyConstraintIJ::postPosICIteration()
{
	xIeJeIe->postPosICIteration();
	yIeJeIe->postPosICIteration();
	ConstraintIJ::postPosICIteration();
}

void MbD::DistancexyConstraintIJ::preAccIC()
{
	xIeJeIe->preAccIC();
	yIeJeIe->preAccIC();
	ConstraintIJ::preAccIC();
}

void MbD::DistancexyConstraintIJ::prePosIC()
{
	xIeJeIe->prePosIC();
	yIeJeIe->prePosIC();
	ConstraintIJ::prePosIC();
}

void MbD::DistancexyConstraintIJ::preVelIC()
{
	xIeJeIe->preVelIC();
	yIeJeIe->preVelIC();
	ConstraintIJ::preVelIC();
}

void MbD::DistancexyConstraintIJ::simUpdateAll()
{
	xIeJeIe->simUpdateAll();
	yIeJeIe->simUpdateAll();
	ConstraintIJ::simUpdateAll();
}

ConstraintType MbD::DistancexyConstraintIJ::type()
{
	return displacement;
}
