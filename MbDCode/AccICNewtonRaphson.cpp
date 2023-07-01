#include "AccICNewtonRaphson.h"
#include "SystemSolver.h"

using namespace MbD;

bool AccICNewtonRaphson::isConverged()
{
	return this->isConvergedToNumericalLimit();
}

void AccICNewtonRaphson::preRun()
{
	std::string str("MbD: Solving for quasi kinematic acceleration.");
	system->logString(str);
	AccNewtonRaphson::preRun();
}
