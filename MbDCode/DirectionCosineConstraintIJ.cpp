#include "DirectionCosineConstraintIJ.h"
#include "EndFramec.h"

using namespace MbD;

DirectionCosineConstraintIJ::DirectionCosineConstraintIJ(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj, int axisi, int axisj) : 
	ConstraintIJ(frmI, frmJ), axisI(axisi), axisJ(axisj)
{
	aAijIeJe = std::make_shared<DirectionCosineIecJec>(frmI, frmJ, axisI, axisJ);
}
