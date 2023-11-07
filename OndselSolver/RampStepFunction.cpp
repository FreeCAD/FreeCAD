#include <algorithm>

#include "RampStepFunction.h"
#include "Constant.h"
#include "Polynomial.h"

using namespace MbD;

MbD::RampStepFunction::RampStepFunction(Symsptr var, std::shared_ptr<std::vector<double>> consts, std::shared_ptr<std::vector<double>> trans)
{

	double x0 = trans->at(0);
	double x1 = trans->at(1);
	double y0 = consts->at(0);
	double y1 = consts->at(1);
	initFunctionsTransitions(var, x0, y0, x1, y1);
}

void MbD::RampStepFunction::arguments(Symsptr args)
{
	auto arguments = args->getTerms();
	auto var = arguments->at(0);
	auto symx0 = arguments->at(1);
	auto symy0 = arguments->at(2);
	auto symx1 = arguments->at(3);
	auto symy1 = arguments->at(4);
	double x0 = symx0->getValue();
	double y0 = symy0->getValue();
	double x1 = symx1->getValue();
	double y1 = symy1->getValue();
	initFunctionsTransitions(var, x0, y0, x1, y1, symx0, symy0, symx1, symy1);
}

void MbD::RampStepFunction::initFunctionsTransitions(Symsptr var, double x0, double y0, double x1, double y1)
{
	auto symx0 = sptrConstant(x0);
	auto symy0 = sptrConstant(y0);
	auto symx1 = sptrConstant(x1);
	auto symy1 = sptrConstant(y1);
	initFunctionsTransitions(var, x0, y0, x1, y1, symx0, symy0, symx1, symy1);
}

void MbD::RampStepFunction::initFunctionsTransitions(Symsptr var, double x0, double y0, double x1, double y1, Symsptr symx0, Symsptr symy0, Symsptr symx1, Symsptr symy1)
{
	//(y - y0)/(x - x0) = (y1 - y0)/(x1 - x0)
	//y = (x - x0)(y1 - y0)/(x1 - x0)  + y0
	//y = (y1 - y0)*x/(x1 - x0) + y0 - (y1 - y0)*x0/(x1 - x0)
	xx = var;
	auto func0 = symy0;
	auto slope = sptrConstant((y1 - y0) / (x1 - x0));
	auto intercept = sptrConstant(y0 - (y1 - y0) * x0 / (x1 - x0));
	auto coeffs = std::make_shared<std::vector<Symsptr>>();
	coeffs->push_back(intercept);
	coeffs->push_back(slope);
	auto func1 = std::make_shared<Polynomial>(var, coeffs);
	auto func2 = symy1;
	functions->push_back(func0);
	functions->push_back(func1);
	functions->push_back(func2);
	transitions->push_back(symx0);
	transitions->push_back(symx1);
}
