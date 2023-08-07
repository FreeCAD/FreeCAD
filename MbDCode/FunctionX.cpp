#include "FunctionX.h"
#include "Constant.h"
#include "Sum.h"

using namespace MbD;

MbD::FunctionX::FunctionX(Symsptr arg) : xx(arg)
{
}

void MbD::FunctionX::arguments(Symsptr args)
{
	//args is a Sum with "terms" containing the actual arguments
	auto sum = std::static_pointer_cast<Sum>(args);
	assert(sum->terms->size() == 1);
	xx = sum->terms->front();
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

void MbD::FunctionX::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	xx->createMbD(mbdSys, mbdUnits);
}
