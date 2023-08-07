#include "DispCompIeqcJeqcO.h"
#include "EndFrameqc.h"

using namespace MbD;

DispCompIeqcJeqcO::DispCompIeqcJeqcO()
{
}

DispCompIeqcJeqcO::DispCompIeqcJeqcO(EndFrmsptr frmi, EndFrmsptr frmj, int axis) : DispCompIeqcJecO(frmi, frmj, axis)
{
}

void DispCompIeqcJeqcO::initializeGlobally()
{
	DispCompIeqcJecO::initializeGlobally();
	priIeJeOpXJ = std::make_shared<FullRow<double>>(3, 0.0);
	priIeJeOpXJ->at(axis) = 1.0;
	ppriIeJeOpEJpEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->ppriOeOpEpE(axis);
}

void DispCompIeqcJeqcO::calcPostDynCorrectorIteration()
{
	DispCompIeqcJecO::calcPostDynCorrectorIteration();
	priIeJeOpEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->priOeOpE(axis);
}

FRowDsptr DispCompIeqcJeqcO::pvaluepXJ()
{
	return priIeJeOpXJ;
}

FRowDsptr DispCompIeqcJeqcO::pvaluepEJ()
{
	return priIeJeOpEJ;
}

FMatDsptr DispCompIeqcJeqcO::ppvaluepEJpEJ()
{
	return ppriIeJeOpEJpEJ;
}
