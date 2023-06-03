#include "DirectionCosineConstraintIJ.h"
#include "DirectionCosineIecJec.h"
#include "EndFramec.h"
#include "CREATE.h"

using namespace MbD;

DirectionCosineConstraintIJ::DirectionCosineConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi, size_t axisj) :
	ConstraintIJ(frmi, frmj), axisI(axisi), axisJ(axisj)
{
}

void DirectionCosineConstraintIJ::initialize()
{
	initaAijIeJe();
}

void MbD::DirectionCosineConstraintIJ::initializeLocally()
{
	aAijIeJe->initializeLocally();
}

void MbD::DirectionCosineConstraintIJ::initializeGlobally()
{
	aAijIeJe->initializeGlobally();
}

void DirectionCosineConstraintIJ::initaAijIeJe()
{
	aAijIeJe = CREATE<DirectionCosineIecJec>::With(frmI, frmJ, axisI, axisJ);
}

void MbD::DirectionCosineConstraintIJ::postInput()
{
	aAijIeJe->postInput();
	Constraint::postInput();
}

void MbD::DirectionCosineConstraintIJ::calcPostDynCorrectorIteration()
{
	aG = aAijIeJe->aAijIeJe - aConstant;
}

void MbD::DirectionCosineConstraintIJ::prePosIC()
{
	aAijIeJe->prePosIC();
	Constraint::prePosIC();
}

void MbD::DirectionCosineConstraintIJ::postPosICIteration()
{
	aAijIeJe->postPosICIteration();
	Item::postPosICIteration();
}

MbD::ConstraintType MbD::DirectionCosineConstraintIJ::type()
{
	return MbD::perpendicular;
}
