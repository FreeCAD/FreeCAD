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
