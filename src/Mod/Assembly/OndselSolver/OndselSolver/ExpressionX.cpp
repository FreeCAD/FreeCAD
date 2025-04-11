/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ExpressionX.h"
#include "Constant.h"

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

Symsptr MbD::ExpressionX::differentiateWRTx()
{
	return expression->differentiateWRT(xx);
}

Symsptr MbD::ExpressionX::differentiateWRT(Symsptr var)
{
	if (this == var.get()) return sptrConstant(1.0);
	return expression->differentiateWRT(var);
}

double MbD::ExpressionX::getValue()
{
	return expression->getValue();
}
