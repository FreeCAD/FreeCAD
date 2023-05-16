#include <memory>

#include "DirectionCosineIecJec.h"
#include "FullColumn.h"

using namespace MbD;

MbD::DirectionCosineIecJec::DirectionCosineIecJec()
{
}

MbD::DirectionCosineIecJec::DirectionCosineIecJec(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	KinematicIeJe(frmi, frmj), axisI(axisi), axisJ(axisj)
{
	aAijIeJe = 0.0;
	aAjOIe = std::make_shared<FullColumn<double>>(3);
	aAjOJe = std::make_shared<FullColumn<double>>(3);
}
