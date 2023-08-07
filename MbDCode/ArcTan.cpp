#include "ArcTan.h"

using namespace MbD;

MbD::ArcTan::ArcTan(Symsptr arg) : FunctionX(arg)
{
}

double MbD::ArcTan::getValue()
{
    return std::atan(xx->getValue());
}

std::ostream& MbD::ArcTan::printOn(std::ostream& s) const
{
	s << "arctan(" << xx << ")";
	return s;
}
