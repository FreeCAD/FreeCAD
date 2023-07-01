#include <string>

#include "KineIntegrator.h"
#include "SystemSolver.h"
#include "Solver.h"

using namespace MbD;

void KineIntegrator::preRun()
{
	system->Solver::logString("MbD: Starting kinematic analysis.");
	QuasiIntegrator::preRun();
}

void KineIntegrator::firstStep()
{
	assert(false);
}

void KineIntegrator::subsequentSteps()
{
	assert(false);
}

void KineIntegrator::nextStep()
{
	assert(false);
}

void KineIntegrator::runInitialConditionTypeSolution()
{
	system->runPosKine();
	system->runVelKine();
	system->runAccKine();
}

void KineIntegrator::iStep(int i)
{
	assert(false);
}

void KineIntegrator::selectOrder()
{
	assert(false);
}
