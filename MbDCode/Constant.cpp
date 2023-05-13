#include "Constant.h"

using namespace MbD;

Constant::Constant()
{
}

Constant::Constant(double val) : Variable(val)
{
}

std::shared_ptr<Symbolic> MbD::Constant::differentiateWRT(std::shared_ptr<Symbolic> var)
{
	return std::make_shared<Constant>(0.0);
}
