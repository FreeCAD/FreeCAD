#include "DirectionCosineIeqctJeqc.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"

using namespace MbD;

DirectionCosineIeqctJeqc::DirectionCosineIeqctJeqc()
{
}

DirectionCosineIeqctJeqc::DirectionCosineIeqctJeqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
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

void MbD::DirectionCosineIeqctJeqc::preVelIC()
{
	Item::preVelIC();
	auto pAjOIept = std::static_pointer_cast<EndFrameqct>(frmI)->pAjOept(axisI);
	pAijIeJept = pAjOIept->dot(aAjOJe);
}

double MbD::DirectionCosineIeqctJeqc::pvaluept()
{
	return pAijIeJept;
}
