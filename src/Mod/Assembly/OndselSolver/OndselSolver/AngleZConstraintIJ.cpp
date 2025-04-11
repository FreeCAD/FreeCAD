/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "AngleZConstraintIJ.h"
#include "AngleZConstraintIqcJqc.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::AngleZConstraintIJ::AngleZConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj) : ConstraintIJ(frmi, frmj)
{
}

std::shared_ptr<AngleZConstraintIJ> MbD::AngleZConstraintIJ::With(EndFrmsptr frmi, EndFrmsptr frmj)
{
	assert(frmi->isEndFrameqc());
	assert(frmj->isEndFrameqc());
	auto angleZConIJ = std::make_shared<AngleZConstraintIqcJqc>(frmi, frmj);
	angleZConIJ->initialize();
	return angleZConIJ;
}

void MbD::AngleZConstraintIJ::calcPostDynCorrectorIteration()
{
	auto thez = thezIeJe->value();
	aG = thez - aConstant;
}

void MbD::AngleZConstraintIJ::initthezIeJe()
{
	assert(false);
}

void MbD::AngleZConstraintIJ::initialize()
{
	ConstraintIJ::initialize();
	this->initthezIeJe();
}

void MbD::AngleZConstraintIJ::initializeGlobally()
{
	thezIeJe->initializeGlobally();
}

void MbD::AngleZConstraintIJ::initializeLocally()
{
	thezIeJe->initializeLocally();
}

void MbD::AngleZConstraintIJ::postInput()
{
	assert(aConstant != std::numeric_limits<double>::min());
	ConstraintIJ::postInput();
}

void MbD::AngleZConstraintIJ::postPosICIteration()
{
	thezIeJe->postPosICIteration();
	ConstraintIJ::postPosICIteration();
}

void MbD::AngleZConstraintIJ::preAccIC()
{
	thezIeJe->preAccIC();
	ConstraintIJ::preAccIC();
}

void MbD::AngleZConstraintIJ::prePosIC()
{
	thezIeJe->prePosIC();
	ConstraintIJ::prePosIC();
}

void MbD::AngleZConstraintIJ::preVelIC()
{
	thezIeJe->preVelIC();
	ConstraintIJ::preVelIC();
}

void MbD::AngleZConstraintIJ::simUpdateAll()
{
	thezIeJe->simUpdateAll();
	ConstraintIJ::simUpdateAll();
}

ConstraintType MbD::AngleZConstraintIJ::type()
{
	return essential;
}
