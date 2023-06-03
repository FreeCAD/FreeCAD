#include <memory>

#include "DirectionCosineIecJec.h"
#include "FullColumn.h"

using namespace MbD;

DirectionCosineIecJec::DirectionCosineIecJec()
{
}

DirectionCosineIecJec::DirectionCosineIecJec(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi, size_t axisj) :
	KinematicIeJe(frmi, frmj), axisI(axisi), axisJ(axisj)
{

}

void MbD::DirectionCosineIecJec::calcPostDynCorrectorIteration()
{
	aAjOIe = frmI->aAjOe(axisI);
	aAjOJe = frmJ->aAjOe(axisJ);
	aAijIeJe = aAjOIe->dot(aAjOJe);
}
