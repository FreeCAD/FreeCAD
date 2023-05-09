#include "DirectionCosineIeqctJeqc.h"

using namespace MbD;

MbD::DirectionCosineIeqctJeqc::DirectionCosineIeqctJeqc()
{
	initialize();
}

void MbD::DirectionCosineIeqctJeqc::initialize()
{
	ppAijIeJepEIpt = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEJpt = std::make_shared<FullRow<double>>(4);
}
