#include "AtPointConstraintIqctJqc.h"
#include "DispCompIeqctJeqcO.h"

using namespace MbD;

AtPointConstraintIqctJqc::AtPointConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	AtPointConstraintIqcJqc(frmi, frmj, axisi)
{
}

void AtPointConstraintIqctJqc::initialize()
{
}

void AtPointConstraintIqctJqc::initriIeJeO()
{
	riIeJeO = std::make_shared<DispCompIeqctJeqcO>();
}

void MbD::AtPointConstraintIqctJqc::calcPostDynCorrectorIteration()
{
}
