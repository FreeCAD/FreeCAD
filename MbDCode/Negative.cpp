#include "Negative.h"
#include "Constant.h"

using namespace MbD;

MbD::Negative::Negative(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Negative::getValue()
{
	return -xx->getValue();
}

Symsptr MbD::Negative::differentiateWRTx()
{
	return std::make_shared<Constant>(-1);
}

std::ostream& MbD::Negative::printOn(std::ostream& s) const
{
	s << "-(" << xx << ")";
	return s;
}
