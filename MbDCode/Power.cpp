#include "Power.h"
#include "Constant.h"
#include "Ln.h"

using namespace MbD;

MbD::Power::Power()
{
}

MbD::Power::Power(Symsptr bse, Symsptr ex) : FunctionXY(bse, ex)
{
}

Symsptr MbD::Power::differentiateWRTx()
{
	auto yminus1 = Symbolic::sum(y, std::make_shared<Constant>(-1.0));
	auto power = Symbolic::raisedTo(x, yminus1);
	auto deriv = Symbolic::times(y, power);
	return deriv->simplified(deriv);
}

Symsptr MbD::Power::differentiateWRTy()
{
	auto lnterm = std::make_shared<Ln>(x);
	auto deriv = Symbolic::times(clonesptr(), lnterm);
	return deriv->simplified();
}

Symsptr MbD::Power::simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	return Symsptr();
}

double MbD::Power::getValue()
{
	return std::pow(x->getValue(), y->getValue());
}
