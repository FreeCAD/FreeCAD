#include "TranslationConstraintIqctJqc.h"
#include "DispCompIeqctJeqcKeqct.h"

using namespace MbD;

TranslationConstraintIqctJqc::TranslationConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisk) :
	TranslationConstraintIqcJqc(frmi, frmj, axisk)
{
}

void TranslationConstraintIqctJqc::initialize()
{
}

void MbD::TranslationConstraintIqctJqc::initriIeJeIe()
{
	riIeJeIe = std::make_shared<DispCompIeqctJeqcKeqct>();
}
