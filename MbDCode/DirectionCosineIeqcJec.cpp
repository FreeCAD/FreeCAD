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
	DirectionCosineIecJec::initialize();
	pAijIeJepEI = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

void DirectionCosineIeqcJec::initializeGlobally()
{
	ppAjOIepEIpEI = std::static_pointer_cast<EndFrameqc>(frmI)->ppAjOepEpE(axisI);
}

void DirectionCosineIeqcJec::calcPostDynCorrectorIteration()
{
	DirectionCosineIecJec::calcPostDynCorrectorIteration();
	pAjOIepEIT = std::static_pointer_cast<EndFrameqc>(frmI)->pAjOepET(axisI);
	for (int i = 0; i < 4; i++)
	{
		pAijIeJepEI->at(i) = pAjOIepEIT->at(i)->dot(aAjOJe);
	}
	for (int i = 0; i < 4; i++)
	{
		auto& ppAijIeJepEIipEI = ppAijIeJepEIpEI->at(i);
		auto& ppAjOIepEIipEI = ppAjOIepEIpEI->at(i);
		for (int j = 0; j < 4; j++)
		{
			ppAijIeJepEIipEI->at(j) = ppAjOIepEIipEI->at(j)->dot(aAjOJe);
		}
	}	
	ppAijIeJepEIpEI->symLowerWithUpper();
}
