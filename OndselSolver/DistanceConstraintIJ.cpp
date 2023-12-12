/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DistanceConstraintIJ.h"
#include "DistanceConstraintIqcJqc.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::DistanceConstraintIJ::DistanceConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj) : ConstraintIJ(frmi, frmj)
{
}

std::shared_ptr<DistanceConstraintIJ> MbD::DistanceConstraintIJ::With(EndFrmsptr frmi, EndFrmsptr frmj)
{
	assert(frmi->isEndFrameqc());
	assert(frmj->isEndFrameqc());
	auto distCon = std::make_shared<DistanceConstraintIqcJqc>(frmi, frmj);
	distCon->init_distIeJe();
	return distCon;
}

void MbD::DistanceConstraintIJ::calcPostDynCorrectorIteration()
{
aG = distIeJe->value() - aConstant;
}

void MbD::DistanceConstraintIJ::init_distIeJe()
{
	assert(false);
}

void MbD::DistanceConstraintIJ::initialize()
{
	ConstraintIJ::initialize();
	this->init_distIeJe();
}

void MbD::DistanceConstraintIJ::initializeGlobally()
{
	distIeJe->initializeGlobally();
}

void MbD::DistanceConstraintIJ::initializeLocally()
{
	distIeJe->initializeLocally();
}

void MbD::DistanceConstraintIJ::postInput()
{
	distIeJe->postInput();
	ConstraintIJ::postInput();
}

void MbD::DistanceConstraintIJ::postPosICIteration()
{
	distIeJe->postPosICIteration();
	ConstraintIJ::postPosICIteration();
}

void MbD::DistanceConstraintIJ::preAccIC()
{
	distIeJe->preAccIC();
	ConstraintIJ::preAccIC();
}

void MbD::DistanceConstraintIJ::prePosIC()
{
	distIeJe->prePosIC();
	ConstraintIJ::prePosIC();
}

void MbD::DistanceConstraintIJ::preVelIC()
{
	distIeJe->preVelIC();
	ConstraintIJ::preVelIC();
}

void MbD::DistanceConstraintIJ::simUpdateAll()
{
	distIeJe->simUpdateAll();
	ConstraintIJ::simUpdateAll();
}

ConstraintType MbD::DistanceConstraintIJ::type()
{
	return ConstraintType::displacement;
}
