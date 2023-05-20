#include "DirectionCosineConstraintIJ.h"
#include "DirectionCosineIecJec.h"
#include "EndFramec.h"

using namespace MbD;

std::shared_ptr<DirectionCosineConstraintIJ> MbD::DirectionCosineConstraintIJ::Create(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj)
{
	auto item = std::make_shared<DirectionCosineConstraintIJ>(frmi, frmj, axisi, axisj);
	item->initialize();
	return item;
}

DirectionCosineConstraintIJ::DirectionCosineConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	ConstraintIJ(frmi, frmj), axisI(axisi), axisJ(axisj)
{
}

void DirectionCosineConstraintIJ::initialize()
{
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
	aAijIeJe = std::make_shared<DirectionCosineIecJec>();
}

void MbD::DirectionCosineConstraintIJ::postInput()
{
}

void MbD::DirectionCosineConstraintIJ::calcPostDynCorrectorIteration()
{
}
