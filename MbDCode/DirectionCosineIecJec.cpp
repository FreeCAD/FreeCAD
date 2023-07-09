#include <memory>

#include "DirectionCosineIecJec.h"
#include "FullColumn.h"

using namespace MbD;

DirectionCosineIecJec::DirectionCosineIecJec()
{
}

DirectionCosineIecJec::DirectionCosineIecJec(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	KinematicIeJe(frmi, frmj), axisI(axisi), axisJ(axisj)
{

}

void DirectionCosineIecJec::calcPostDynCorrectorIteration()
{
	aAjOIe = frmI->aAjOe(axisI);
	aAjOJe = frmJ->aAjOe(axisJ);
	aAijIeJe = aAjOIe->dot(aAjOJe);
}

double MbD::DirectionCosineIecJec::value()
{
	return aAijIeJe;
}
