#include "IndependentVariable.h"
#include "Constant.h"

using namespace MbD;

IndependentVariable::IndependentVariable()
{
}

Symsptr IndependentVariable::differentiateWRT(Symsptr sptr, Symsptr var)
{
	if (this == var.get()) {
		return std::make_shared<Constant>(1.0);
	}
	else {
		return std::make_shared<Constant>(0.0);
	}
}
