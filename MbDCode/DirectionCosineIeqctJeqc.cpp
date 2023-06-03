#include "DirectionCosineIeqctJeqc.h"
#include "EndFrameqc.h"

using namespace MbD;

DirectionCosineIeqctJeqc::DirectionCosineIeqctJeqc()
{
}

DirectionCosineIeqctJeqc::DirectionCosineIeqctJeqc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi, size_t axisj) :
	DirectionCosineIeqcJeqc(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineIeqctJeqc::initialize()
{
	DirectionCosineIeqcJeqc::initialize();
	ppAijIeJepEIpt = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEJpt = std::make_shared<FullRow<double>>(4);
}

void MbD::DirectionCosineIeqctJeqc::initializeGlobally()
{
	ppAjOJepEJpEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->ppAjOepEpE(axisJ);
}

void MbD::DirectionCosineIeqctJeqc::calcPostDynCorrectorIteration()
{
	//"ppAjOIepEIpEI is not longer constant and must be set before any calculation."

	ppAjOIepEIpEI = std::static_pointer_cast<EndFrameqc>(frmI)->ppAjOepEpE(axisI);
	DirectionCosineIeqcJeqc::calcPostDynCorrectorIteration();
}
