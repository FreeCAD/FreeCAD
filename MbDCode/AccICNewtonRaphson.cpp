#include "AccICNewtonRaphson.h"
#include "SystemSolver.h"

bool MbD::AccICNewtonRaphson::isConverged()
{
	return this->isConvergedToNumericalLimit();
}

void MbD::AccICNewtonRaphson::preRun()
{
	std::string str("MbD: Solving for quasi kinematic acceleration.");
	system->logString(str);
	AccNewtonRaphson::preRun();
}
