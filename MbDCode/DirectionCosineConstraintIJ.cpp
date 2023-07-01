#include "DirectionCosineConstraintIJ.h"
#include "DirectionCosineIecJec.h"
#include "EndFramec.h"
#include "CREATE.h"

using namespace MbD;

DirectionCosineConstraintIJ::DirectionCosineConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
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
	Constraint::postInput();
}

void DirectionCosineConstraintIJ::calcPostDynCorrectorIteration()
{
	aG = aAijIeJe->aAijIeJe - aConstant;
}

void DirectionCosineConstraintIJ::prePosIC()
{
	aAijIeJe->prePosIC();
	Constraint::prePosIC();
}

void DirectionCosineConstraintIJ::postPosICIteration()
{
	aAijIeJe->postPosICIteration();
	Item::postPosICIteration();
}

ConstraintType DirectionCosineConstraintIJ::type()
{
	return perpendicular;
}

void DirectionCosineConstraintIJ::preVelIC()
{
	aAijIeJe->preVelIC();
	Item::preVelIC();
}

void DirectionCosineConstraintIJ::preAccIC()
{
	aAijIeJe->preAccIC();
	Constraint::preAccIC();
}
