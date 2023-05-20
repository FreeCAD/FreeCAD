#include "DirectionCosineIeqcJec.h"
#include "EndFrameqc.h"

using namespace MbD;

DirectionCosineIeqcJec::DirectionCosineIeqcJec()
{
}

DirectionCosineIeqcJec::DirectionCosineIeqcJec(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	DirectionCosineIecJec(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineIeqcJec::initialize()
{
	pAijIeJepEI = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::DirectionCosineIeqcJec::initializeGlobally()
{
	ppAjOIepEIpEI = std::static_pointer_cast<EndFrameqc>(frmI)->ppAjOepEpE(axisI);
}

void MbD::DirectionCosineIeqcJec::calcPostDynCorrectorIteration()
{
}
