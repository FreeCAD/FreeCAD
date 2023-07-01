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
	DirectionCosineIeqcJec::initialize();
	pAijIeJepEJ = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppAijIeJepEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}

void DirectionCosineIeqcJeqc::initializeGlobally()
{
	DirectionCosineIeqcJec::initializeGlobally();
	ppAjOJepEJpEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->ppAjOepEpE(axisJ);
}

void DirectionCosineIeqcJeqc::calcPostDynCorrectorIteration()
{
	DirectionCosineIeqcJec::calcPostDynCorrectorIteration();
	pAjOJepEJT = std::static_pointer_cast<EndFrameqc>(frmJ)->pAjOepET(axisJ);
	for (int i = 0; i < 4; i++)
	{
		pAijIeJepEJ->at(i) = aAjOIe->dot(pAjOJepEJT->at(i));
	}
	for (int i = 0; i < 4; i++)
	{
		auto& ppAijIeJepEIipEJ = ppAijIeJepEIpEJ->at(i);
		for (int j = 0; j < 4; j++)
		{
			ppAijIeJepEIipEJ->at(j) = pAjOIepEIT->at(i)->dot(pAjOJepEJT->at(j));
		}
	}
	for (int i = 0; i < 4; i++)
	{
		auto& ppAijIeJepEJipEJ = ppAijIeJepEJpEJ->at(i);
		auto& ppAjOJepEJipEJ = ppAjOJepEJpEJ->at(i);
		for (int j = 0; j < 4; j++)
		{
			ppAijIeJepEJipEJ->at(j) = aAjOIe->dot(ppAjOJepEJipEJ->at(j));
		}
	}
	ppAijIeJepEJpEJ->symLowerWithUpper();
}

FRowDsptr DirectionCosineIeqcJeqc::pvaluepEJ()
{
	return pAijIeJepEJ;
}

FMatDsptr DirectionCosineIeqcJeqc::ppvaluepEJpEJ()
{
	return ppAijIeJepEJpEJ;
}
