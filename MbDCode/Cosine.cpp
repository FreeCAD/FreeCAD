#include "Cosine.h"
#include "Sine.h"
#include "Negative.h"

using namespace MbD;

MbD::Cosine::Cosine(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Cosine::getValue()
{
    return std::cos(xx->getValue());
}

Symsptr MbD::Cosine::differentiateWRTx()
{
    return std::make_shared<Negative>(std::make_shared<Sine>(xx));
}

std::ostream& MbD::Cosine::printOn(std::ostream& s) const
{
	s << "cos(" << xx << ")";
	return s;
}
