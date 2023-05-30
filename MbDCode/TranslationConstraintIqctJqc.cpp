#include "TranslationConstraintIqctJqc.h"
#include "DispCompIeqctJeqcKeqct.h"
#include "CREATE.h"

using namespace MbD;

TranslationConstraintIqctJqc::TranslationConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	TranslationConstraintIqcJqc(frmi, frmj, axisi)
{
}

void MbD::TranslationConstraintIqctJqc::initriIeJeIe()
{
	riIeJeIe = CREATE<DispCompIeqctJeqcKeqct>::With(frmI, frmJ, frmI, axisI);
}

MbD::ConstraintType MbD::TranslationConstraintIqctJqc::type()
{
	return MbD::essential;
}
