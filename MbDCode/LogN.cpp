#include "LogN.h"

using namespace MbD;

MbD::LogN::LogN(Symsptr arg) : FunctionX(arg)
{
}

double MbD::LogN::getValue()
{
    return std::log(xx->getValue());
}

std::ostream& MbD::LogN::printOn(std::ostream& s) const
{
	s << "ln(" << xx << ")";
	return s;
}
