#include "DispCompIeqctJeqcO.h"
#include "EndFrameqct.h"

MbD::DispCompIeqctJeqcO::DispCompIeqctJeqcO()
{
}

MbD::DispCompIeqctJeqcO::DispCompIeqctJeqcO(EndFrmcptr frmi, EndFrmcptr frmj, int axis) : DispCompIeqcJeqcO(frmi, frmj, axis)
{
}

void MbD::DispCompIeqctJeqcO::initializeGlobally()
{
	//ToDo: Check why not using super classes.
	ppriIeJeOpEJpEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->ppriOeOpEpE(axis);
}

void MbD::DispCompIeqctJeqcO::calcPostDynCorrectorIteration()
{
	//"ppriIeJeOpEIpEI is not a constant now."
	DispCompIeqcJeqcO::calcPostDynCorrectorIteration();
	ppriIeJeOpEIpEI = std::static_pointer_cast<EndFrameqc>(frmI)->ppriOeOpEpE(axis)->negated();
}
