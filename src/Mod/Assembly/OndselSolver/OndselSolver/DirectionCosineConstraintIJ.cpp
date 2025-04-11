/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DirectionCosineConstraintIJ.h"
#include "DirectionCosineIecJec.h"
#include "EndFramec.h"
#include "CREATE.h"

using namespace MbD;

DirectionCosineConstraintIJ::DirectionCosineConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj) :
	ConstraintIJ(frmi, frmj), axisI(axisi), axisJ(axisj)
{
}

void DirectionCosineConstraintIJ::initialize()
{
	ConstraintIJ::initialize();
	initaAijIeJe();
}

void DirectionCosineConstraintIJ::initializeLocally()
{
	aAijIeJe->initializeLocally();
}

void DirectionCosineConstraintIJ::initializeGlobally()
{
	aAijIeJe->initializeGlobally();
}

void DirectionCosineConstraintIJ::initaAijIeJe()
{
	aAijIeJe = CREATE<DirectionCosineIecJec>::With(frmI, frmJ, axisI, axisJ);
}

void DirectionCosineConstraintIJ::postInput()
{
	aAijIeJe->postInput();
	ConstraintIJ::postInput();
}

void DirectionCosineConstraintIJ::calcPostDynCorrectorIteration()
{
	aG = aAijIeJe->aAijIeJe - aConstant;
}

void DirectionCosineConstraintIJ::prePosIC()
{
	aAijIeJe->prePosIC();
	ConstraintIJ::prePosIC();
}

void DirectionCosineConstraintIJ::postPosICIteration()
{
	aAijIeJe->postPosICIteration();
	ConstraintIJ::postPosICIteration();
}

ConstraintType DirectionCosineConstraintIJ::type()
{
	return perpendicular;
}

void DirectionCosineConstraintIJ::preVelIC()
{
	aAijIeJe->preVelIC();
	ConstraintIJ::preVelIC();
}

void MbD::DirectionCosineConstraintIJ::simUpdateAll()
{
	aAijIeJe->simUpdateAll();
	ConstraintIJ::simUpdateAll();
}

void DirectionCosineConstraintIJ::preAccIC()
{
	aAijIeJe->preAccIC();
	ConstraintIJ::preAccIC();
}
