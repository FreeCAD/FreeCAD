#include "AtPointConstraintIqctJqc.h"
#include "DispCompIeqctJeqcO.h"
#include "CREATE.h"

using namespace MbD;

AtPointConstraintIqctJqc::AtPointConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	AtPointConstraintIqcJqc(frmi, frmj, axisi)
{
}

void MbD::AtPointConstraintIqctJqc::initializeGlobally()
{
	riIeJeO->initializeGlobally();
	ppGpEJpEJ = (std::static_pointer_cast<DispCompIeqctJeqcO>(riIeJeO))->ppriIeJeOpEJpEJ;
}

void AtPointConstraintIqctJqc::initriIeJeO()
{
	riIeJeO = CREATE<DispCompIeqctJeqcO>::With(frmI, frmJ, axis);
}

void MbD::AtPointConstraintIqctJqc::calcPostDynCorrectorIteration()
{
	//"ppGpEIpEI is no longer constant."

	ppGpEIpEI = std::static_pointer_cast<DispCompIeqctJeqcO>(riIeJeO)->ppriIeJeOpEIpEI;
	AtPointConstraintIqcJqc::calcPostDynCorrectorIteration();
}

MbD::ConstraintType MbD::AtPointConstraintIqctJqc::type()
{
	return MbD::essential;
}
