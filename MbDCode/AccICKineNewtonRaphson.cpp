#include "AccICKineNewtonRaphson.h"
#include "SystemSolver.h"

void MbD::AccICKineNewtonRaphson::initializeGlobally()
{
	AccNewtonRaphson::initializeGlobally();
	iterMax = system->iterMaxAccKine;
	dxTol = system->errorTolAccKine;
}

void MbD::AccICKineNewtonRaphson::preRun()
{
	std::string str("MbD: Solving for quasi kinematic acceleration.");
	system->logString(str);
	AccNewtonRaphson::preRun();
}
