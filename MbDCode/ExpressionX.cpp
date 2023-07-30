#include "ExpressionX.h"

using namespace MbD;

void MbD::ExpressionX::xexpression(Symsptr arg, Symsptr func)
{
	//"
	//Future modification :
	//Check that func is a function of arg.
	//No need for self to be dependent of arg since self is dependent of func which is indirectly
	//dependent of of arg.
	//"

	xx = arg;
	expression = func;
}

Symsptr MbD::ExpressionX::differentiateWRT(Symsptr var)
{
	return expression->differentiateWRT(var);
}

double MbD::ExpressionX::getValue()
{
	return expression->getValue();
}
