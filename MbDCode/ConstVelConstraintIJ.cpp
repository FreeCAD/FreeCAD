#include "ConstVelConstraintIJ.h"
#include "DirectionCosineIecJec.h"

using namespace MbD;

ConstVelConstraintIJ::ConstVelConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj) : ConstraintIJ(frmi, frmj)
{
}

void ConstVelConstraintIJ::calcPostDynCorrectorIteration()
{
	aG = aA01IeJe->aAijIeJe + aA10IeJe->aAijIeJe - aConstant;
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
	Constraint::postInput();
}

ConstraintType ConstVelConstraintIJ::type()
{
	return ConstraintType();
}

void ConstVelConstraintIJ::postPosICIteration()
{
	aA01IeJe->postPosICIteration();
	aA10IeJe->postPosICIteration();
	Item::postPosICIteration();
}

void ConstVelConstraintIJ::preAccIC()
{
	aA01IeJe->preAccIC();
	aA10IeJe->preAccIC();
	Constraint::preAccIC();
}

void ConstVelConstraintIJ::prePosIC()
{
	aA01IeJe->prePosIC();
	aA10IeJe->prePosIC();
	Constraint::prePosIC();
}

void ConstVelConstraintIJ::preVelIC()
{
	aA01IeJe->preVelIC();
	aA10IeJe->preVelIC();
	Item::preVelIC();
}

void ConstVelConstraintIJ::simUpdateAll()
{
	aA01IeJe->simUpdateAll();
	aA10IeJe->simUpdateAll();
	Item::simUpdateAll();
}
