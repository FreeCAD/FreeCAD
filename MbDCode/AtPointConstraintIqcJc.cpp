#include "AtPointConstraintIqcJc.h"
#include "DispCompIeqcJecO.h"

using namespace MbD;

AtPointConstraintIqcJc::AtPointConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	AtPointConstraintIJ(frmi, frmj, axisi)
{
}

void AtPointConstraintIqcJc::initialize()
{
}

void AtPointConstraintIqcJc::initriIeJeO()
{
	riIeJeO = std::make_shared<DispCompIeqcJecO>();
}

void MbD::AtPointConstraintIqcJc::calcPostDynCorrectorIteration()
{
}
