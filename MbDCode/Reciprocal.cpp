#include "Reciprocal.h"
#include "Negative.h"
#include "Power.h"
#include "Constant.h"

using namespace MbD;

MbD::Reciprocal::Reciprocal(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Reciprocal::getValue()
{
	return 1.0 / xx->getValue();
}

Symsptr MbD::Reciprocal::differentiateWRTx()
{
	auto two = std::make_shared<Constant>(2);
	auto sq = std::make_shared<Power>(xx, two);
	return std::make_shared<Negative>(sq);
}

std::ostream& MbD::Reciprocal::printOn(std::ostream& s) const
{
	s << "/(" << xx << ")";
	return s;
}
