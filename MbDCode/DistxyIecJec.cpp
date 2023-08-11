/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DistxyIecJec.h"

using namespace MbD;

MbD::DistxyIecJec::DistxyIecJec()
{
}

MbD::DistxyIecJec::DistxyIecJec(EndFrmsptr frmi, EndFrmsptr frmj) : KinematicIeJe(frmi, frmj)
{
}

void MbD::DistxyIecJec::calcPostDynCorrectorIteration()
{
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	distxy = std::sqrt(x * x + (y * y));
}

void MbD::DistxyIecJec::initialize()
{
	KinematicIeJe::initialize();
	this->init_xyIeJeIe();
}

void MbD::DistxyIecJec::initializeGlobally()
{
	xIeJeIe->initializeGlobally();
	yIeJeIe->initializeGlobally();
}

void MbD::DistxyIecJec::initializeLocally()
{
	xIeJeIe->initializeLocally();
	yIeJeIe->initializeLocally();
}

void MbD::DistxyIecJec::init_xyIeJeIe()
{
	assert(false);
}

void MbD::DistxyIecJec::postInput()
{
	xIeJeIe->postInput();
	yIeJeIe->postInput();
	KinematicIeJe::postInput();
}

void MbD::DistxyIecJec::postPosICIteration()
{
	xIeJeIe->postPosICIteration();
	yIeJeIe->postPosICIteration();
	KinematicIeJe::postPosICIteration();
}

void MbD::DistxyIecJec::preAccIC()
{
	xIeJeIe->preAccIC();
	yIeJeIe->preAccIC();
	KinematicIeJe::preAccIC();
}

void MbD::DistxyIecJec::prePosIC()
{
	xIeJeIe->prePosIC();
	yIeJeIe->prePosIC();
	KinematicIeJe::prePosIC();
}

void MbD::DistxyIecJec::preVelIC()
{
	xIeJeIe->preVelIC();
	yIeJeIe->preVelIC();
	KinematicIeJe::preVelIC();
}

void MbD::DistxyIecJec::simUpdateAll()
{
	xIeJeIe->simUpdateAll();
	yIeJeIe->simUpdateAll();
	KinematicIeJe::simUpdateAll();
}

double MbD::DistxyIecJec::value()
{
	return distxy;
}
