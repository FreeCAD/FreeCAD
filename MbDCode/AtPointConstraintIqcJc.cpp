#include "AtPointConstraintIqcJc.h"
#include "DispCompIeqcJecO.h"
#include "CREATE.h"
#include "EndFrameqc.h"

using namespace MbD;

AtPointConstraintIqcJc::AtPointConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi) :
	AtPointConstraintIJ(frmi, frmj, axisi)
{
}

void MbD::AtPointConstraintIqcJc::initializeGlobally()
{
	AtPointConstraintIJ::initializeGlobally();
	ppGpEIpEI = (std::static_pointer_cast<DispCompIeqcJecO>(riIeJeO))->ppriIeJeOpEIpEI;
}

void AtPointConstraintIqcJc::initriIeJeO()
{
	riIeJeO = CREATE<DispCompIeqcJecO>::With(frmI, frmJ, axis);
}

void MbD::AtPointConstraintIqcJc::calcPostDynCorrectorIteration()
{
	AtPointConstraintIJ::calcPostDynCorrectorIteration();
	pGpEI = std::static_pointer_cast<DispCompIeqcJecO>(riIeJeO)->priIeJeOpEI;
}

void MbD::AtPointConstraintIqcJc::useEquationNumbers()
{
	iqXIminusOnePlusAxis = std::static_pointer_cast<EndFrameqc>(frmI)->iqX() - 1 + axis;
	iqEI = std::static_pointer_cast<EndFrameqc>(frmI)->iqE();
}
