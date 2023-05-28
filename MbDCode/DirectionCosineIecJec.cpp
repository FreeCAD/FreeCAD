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

void MbD::DirectionCosineIecJec::initialize()
{
	aAijIeJe = 0.0;
	aAjOIe = std::make_shared<FullColumn<double>>(3);
	aAjOJe = std::make_shared<FullColumn<double>>(3);
}

void MbD::DirectionCosineIecJec::calcPostDynCorrectorIteration()
{
}
