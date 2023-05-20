#include "TranslationConstraintIqcJc.h"
#include "DispCompIeqcJecKeqc.h"

using namespace MbD;

TranslationConstraintIqcJc::TranslationConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisk) :
	TranslationConstraintIJ(frmi, frmj, axisk)
{
}

void TranslationConstraintIqcJc::initialize()
{
}

void MbD::TranslationConstraintIqcJc::initriIeJeIe()
{
    riIeJeIe = std::make_shared<DispCompIeqcJecKeqc>();
}
