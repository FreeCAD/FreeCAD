#include "DirectionCosineIeqcJec.h"

using namespace MbD;

MbD::DirectionCosineIeqcJec::DirectionCosineIeqcJec()
{
	initialize();
}

void MbD::DirectionCosineIeqcJec::initialize()
{
	pAijIeJepEI = std::make_shared<FullRow<double>>(4);
	ppAijIeJepEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}
