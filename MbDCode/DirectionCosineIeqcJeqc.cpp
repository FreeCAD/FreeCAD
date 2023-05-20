#include "DirectionCosineIeqcJeqc.h"
#include "EndFrameqc.h"

using namespace MbD;

DirectionCosineIeqcJeqc::DirectionCosineIeqcJeqc()
{
}

DirectionCosineIeqcJeqc::DirectionCosineIeqcJeqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	DirectionCosineIeqcJec(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineIeqcJeqc::initialize()
{
	pAijIeJepEJ = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppAijIeJepEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::DirectionCosineIeqcJeqc::initializeGlobally()
{
	DirectionCosineIeqcJec::initializeGlobally();
	ppAjOJepEJpEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->ppAjOepEpE(axisJ);
}

void MbD::DirectionCosineIeqcJeqc::calcPostDynCorrectorIteration()
{
}
