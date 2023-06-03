#include "TranslationConstraintIqcJqc.h"
#include "DispCompIeqcJeqcKeqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

TranslationConstraintIqcJqc::TranslationConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi) :
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
	iqXJ = std::static_pointer_cast<EndFrameqc>(frmJ)->iqX();
	iqEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->iqE();
}
