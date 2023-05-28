#include "TranslationConstraintIqcJqc.h"
#include "DispCompIeqcJeqcKeqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

TranslationConstraintIqcJqc::TranslationConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	TranslationConstraintIqcJc(frmi, frmj, axisi)
{
}

void MbD::TranslationConstraintIqcJqc::initriIeJeIe()
{
	riIeJeIe = CREATE<DispCompIeqcJeqcKeqc>::With(frmI, frmJ, frmI, axisI);
}

void MbD::TranslationConstraintIqcJqc::calcPostDynCorrectorIteration()
{
}

void MbD::TranslationConstraintIqcJqc::useEquationNumbers()
{
	TranslationConstraintIqcJc::useEquationNumbers();
	iqEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->iqE();
}
