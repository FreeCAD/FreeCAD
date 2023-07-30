#include "FunctionX.h"
#include "Constant.h"

using namespace MbD;

MbD::FunctionX::FunctionX(Symsptr arg) : xx(arg)
{
}

Symsptr MbD::FunctionX::differentiateWRT(Symsptr var)
{
	if (this == var.get()) return std::make_shared<Constant>(1.0);
	auto dfdx = differentiateWRTx();
	auto dxdvar = xx->differentiateWRT(var);
	return Symbolic::times(dfdx, dxdvar);
}

Symsptr MbD::FunctionX::differentiateWRTx()
{
	assert(false);
	return Symsptr();
}
