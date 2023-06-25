#include "DispCompIeqctJeqcO.h"
#include "EndFrameqct.h"

using namespace MbD;

MbD::DispCompIeqctJeqcO::DispCompIeqctJeqcO()
{
}

MbD::DispCompIeqctJeqcO::DispCompIeqctJeqcO(EndFrmcptr frmi, EndFrmcptr frmj, int axis) : DispCompIeqcJeqcO(frmi, frmj, axis)
{
}

void MbD::DispCompIeqctJeqcO::initializeGlobally()
{
	//ToDo: Check why not using super classes.
	ppriIeJeOpEJpEJ = std::static_pointer_cast<EndFrameqct>(frmJ)->ppriOeOpEpE(axis);
}

void MbD::DispCompIeqctJeqcO::calcPostDynCorrectorIteration()
{
	//"ppriIeJeOpEIpEI is not a constant now."
	DispCompIeqcJeqcO::calcPostDynCorrectorIteration();
	ppriIeJeOpEIpEI = std::static_pointer_cast<EndFrameqct>(frmI)->ppriOeOpEpE(axis)->negated();
}

void MbD::DispCompIeqctJeqcO::preVelIC()
{
	Item::preVelIC();
	priIeJeOpt = -(std::static_pointer_cast<EndFrameqct>(frmI)->priOeOpt(axis));
}

double MbD::DispCompIeqctJeqcO::pvaluept()
{
	return priIeJeOpt;
}

void MbD::DispCompIeqctJeqcO::preAccIC()
{
	Item::preAccIC();
	ppriIeJeOpEIpt = (std::static_pointer_cast<EndFrameqct>(frmI)->ppriOeOpEpt(axis))->negated();
	ppriIeJeOptpt = -(std::static_pointer_cast<EndFrameqct>(frmI)->ppriOeOptpt(axis));
}
