#include "AtPointConstraintIqcJqc.h"
#include "DispCompIeqcJeqcO.h"

using namespace MbD;

AtPointConstraintIqcJqc::AtPointConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	AtPointConstraintIqcJc(frmi, frmj, axisi)
{
}

void AtPointConstraintIqcJqc::initialize()
{
}

void AtPointConstraintIqcJqc::initriIeJeO()
{
	riIeJeO = std::make_shared<DispCompIeqcJeqcO>();
}

void MbD::AtPointConstraintIqcJqc::calcPostDynCorrectorIteration()
{
}
