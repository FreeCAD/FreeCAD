#include "DirectionCosineIeqcJeqc.h"

using namespace MbD;

MbD::DirectionCosineIeqcJeqc::DirectionCosineIeqcJeqc()
{
	initialize();
}

void MbD::DirectionCosineIeqcJeqc::initialize()
{
	pAijIeJepEJ = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppAijIeJepEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}
