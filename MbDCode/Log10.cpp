#include "Log10.h"

using namespace MbD;

MbD::Log10::Log10(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Log10::getValue()
{
    return std::log(xx->getValue());
}

std::ostream& MbD::Log10::printOn(std::ostream& s) const
{
	s << "lg(" << xx << ")";
	return s;
}
