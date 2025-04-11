/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ConstVelConstraintIJ.h"
#include "DirectionCosineIecJec.h"
#include "ConstVelConstraintIqcJqc.h"
#include "EndFrameqc.h"

using namespace MbD;

ConstVelConstraintIJ::ConstVelConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj) : ConstraintIJ(frmi, frmj)
{
}

std::shared_ptr<ConstVelConstraintIJ> MbD::ConstVelConstraintIJ::With(EndFrmsptr frmi, EndFrmsptr frmj)
{
	assert(frmi->isEndFrameqc());
	assert(frmj->isEndFrameqc());
	auto constVelCon = std::make_shared<ConstVelConstraintIqcJqc>(frmi, frmj);
	constVelCon->initA01IeJe();
	constVelCon->initA10IeJe();
	return constVelCon;
}

void ConstVelConstraintIJ::calcPostDynCorrectorIteration()
{
	aG = aA01IeJe->aAijIeJe + aA10IeJe->aAijIeJe - aConstant;
}

void MbD::ConstVelConstraintIJ::initA01IeJe()
{
	assert(false);
}

void MbD::ConstVelConstraintIJ::initA10IeJe()
{
	assert(false);
}

void ConstVelConstraintIJ::initialize()
{
	this->initA01IeJe();
	this->initA10IeJe();
}

void ConstVelConstraintIJ::initializeGlobally()
{
	aA01IeJe->initializeGlobally();
	aA10IeJe->initializeGlobally();
}

void ConstVelConstraintIJ::initializeLocally()
{
	aA01IeJe->initializeLocally();
	aA10IeJe->initializeLocally();
}

void ConstVelConstraintIJ::postInput()
{
	aA01IeJe->postInput();
	aA10IeJe->postInput();
	ConstraintIJ::postInput();
}

void ConstVelConstraintIJ::postPosICIteration()
{
	aA01IeJe->postPosICIteration();
	aA10IeJe->postPosICIteration();
	ConstraintIJ::postPosICIteration();
}

void ConstVelConstraintIJ::preAccIC()
{
	aA01IeJe->preAccIC();
	aA10IeJe->preAccIC();
	ConstraintIJ::preAccIC();
}

void ConstVelConstraintIJ::prePosIC()
{
	aA01IeJe->prePosIC();
	aA10IeJe->prePosIC();
	ConstraintIJ::prePosIC();
}

void ConstVelConstraintIJ::preVelIC()
{
	aA01IeJe->preVelIC();
	aA10IeJe->preVelIC();
	ConstraintIJ::preVelIC();
}

void ConstVelConstraintIJ::simUpdateAll()
{
	aA01IeJe->simUpdateAll();
	aA10IeJe->simUpdateAll();
	ConstraintIJ::simUpdateAll();
}
