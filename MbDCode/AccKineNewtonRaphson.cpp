#include "AccKineNewtonRaphson.h"
#include "SystemSolver.h"

void MbD::AccKineNewtonRaphson::initializeGlobally()
{
	AccNewtonRaphson::initializeGlobally();
	iterMax = system->iterMaxAccKine;
	dxTol = system->errorTolAccKine;
}

void MbD::AccKineNewtonRaphson::preRun()
{
	std::string str("MbD: Solving for kinematic acceleration.");
	system->logString(str);
	AccNewtonRaphson::preRun();
}
