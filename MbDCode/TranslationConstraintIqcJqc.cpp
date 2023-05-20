#include "TranslationConstraintIqcJqc.h"
#include "DispCompIeqcJeqcKeqc.h"

using namespace MbD;

TranslationConstraintIqcJqc::TranslationConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisk) :
	TranslationConstraintIqcJc(frmi, frmj, axisk)
{
}

void TranslationConstraintIqcJqc::initialize()
{
}

void MbD::TranslationConstraintIqcJqc::initriIeJeIe()
{
	riIeJeIe = std::make_shared<DispCompIeqcJeqcKeqc>();
}
