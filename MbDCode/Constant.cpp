#include "Constant.h"

using namespace MbD;

Constant::Constant()
{
}

Constant::Constant(double val) : Variable(val)
{
}

Symsptr Constant::differentiateWRT(Symsptr sptr, Symsptr var)
{
	return std::make_shared<Constant>(0.0);
}

bool Constant::isConstant()
{
	return true;
}

std::ostream& Constant::printOn(std::ostream& s) const
{
	return s << this->value;
}
