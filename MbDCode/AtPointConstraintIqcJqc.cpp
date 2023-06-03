#include "AtPointConstraintIqcJqc.h"
#include "DispCompIeqcJeqcO.h"
#include "CREATE.h"
#include "EndFrameqc.h"

using namespace MbD;

AtPointConstraintIqcJqc::AtPointConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi) :
	AtPointConstraintIqcJc(frmi, frmj, axisi)
{
}

void MbD::AtPointConstraintIqcJqc::initializeGlobally()
{
	AtPointConstraintIqcJc::initializeGlobally();
	ppGpEJpEJ = (std::static_pointer_cast<DispCompIeqcJeqcO>(riIeJeO))->ppriIeJeOpEJpEJ;
}

void AtPointConstraintIqcJqc::initriIeJeO()
{
	riIeJeO = CREATE<DispCompIeqcJeqcO>::With(frmI, frmJ, axis);
}

void MbD::AtPointConstraintIqcJqc::calcPostDynCorrectorIteration()
{
	AtPointConstraintIqcJc::calcPostDynCorrectorIteration();
	pGpEJ = std::static_pointer_cast<DispCompIeqcJeqcO>(riIeJeO)->priIeJeOpEJ;
}

void MbD::AtPointConstraintIqcJqc::useEquationNumbers()
{
	AtPointConstraintIqcJc::useEquationNumbers();
	iqXJminusOnePlusAxis = std::static_pointer_cast<EndFrameqc>(frmJ)->iqX() - 1 + axis;
	iqEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->iqE();
}
