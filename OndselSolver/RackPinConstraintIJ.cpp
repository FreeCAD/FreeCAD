/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "RackPinConstraintIJ.h"
#include "RackPinConstraintIqcJqc.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::RackPinConstraintIJ::RackPinConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj) : ConstraintIJ(frmi, frmj)
{
}

std::shared_ptr<RackPinConstraintIJ> MbD::RackPinConstraintIJ::With(EndFrmsptr frmi, EndFrmsptr frmj)
{
	assert(frmi->isEndFrameqc());
	assert(frmj->isEndFrameqc());
	auto rackPinCon = std::make_shared<RackPinConstraintIqcJqc>(frmi, frmj);
	rackPinCon->initxIeJeIe();
	rackPinCon->initthezIeJe();
	return rackPinCon;
}

void MbD::RackPinConstraintIJ::calcPostDynCorrectorIteration()
{
	auto x = xIeJeIe->value();
	auto thez = thezIeJe->value();
	aG = x + (pitchRadius * thez) - aConstant;
}

void MbD::RackPinConstraintIJ::init_xthez()
{
	assert(false);
}

void MbD::RackPinConstraintIJ::initxIeJeIe()
{
	assert(false);
}

void MbD::RackPinConstraintIJ::initthezIeJe()
{
	assert(false);
}

void MbD::RackPinConstraintIJ::initialize()
{
	ConstraintIJ::initialize();
	this->init_xthez();
}

void MbD::RackPinConstraintIJ::initializeGlobally()
{
	xIeJeIe->initializeGlobally();
	thezIeJe->initializeGlobally();
}

void MbD::RackPinConstraintIJ::initializeLocally()
{
	xIeJeIe->initializeLocally();
	thezIeJe->initializeLocally();
}

void MbD::RackPinConstraintIJ::postInput()
{
	xIeJeIe->postInput();
	thezIeJe->postInput();
	if (aConstant == std::numeric_limits<double>::min()) {
		aConstant = xIeJeIe->value() + (pitchRadius * thezIeJe->value());
	}
	ConstraintIJ::postInput();
}

void MbD::RackPinConstraintIJ::postPosICIteration()
{
	xIeJeIe->postPosICIteration();
	thezIeJe->postPosICIteration();
	ConstraintIJ::postPosICIteration();
}

void MbD::RackPinConstraintIJ::preAccIC()
{
	xIeJeIe->preAccIC();
	thezIeJe->preAccIC();
	ConstraintIJ::preAccIC();
}

void MbD::RackPinConstraintIJ::prePosIC()
{
	xIeJeIe->prePosIC();
	thezIeJe->prePosIC();
	ConstraintIJ::prePosIC();
}

void MbD::RackPinConstraintIJ::preVelIC()
{
	xIeJeIe->preVelIC();
	thezIeJe->preVelIC();
	ConstraintIJ::preVelIC();
}

void MbD::RackPinConstraintIJ::simUpdateAll()
{
	xIeJeIe->simUpdateAll();
	thezIeJe->simUpdateAll();
	ConstraintIJ::simUpdateAll();
}
