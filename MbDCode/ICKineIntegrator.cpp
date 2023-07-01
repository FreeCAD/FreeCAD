#include "ICKineIntegrator.h"
#include "SystemSolver.h"

using namespace MbD;

void ICKineIntegrator::runInitialConditionTypeSolution()
{
	system->runPosICKine();
	system->runVelICKine();
	system->runAccICKine();
}

void ICKineIntegrator::iStep(int i)
{
	assert(false);
}

void ICKineIntegrator::selectOrder()
{
	assert(false);
}

void ICKineIntegrator::preRun()
{
	system->Solver::logString("MbD: Starting quasi kinematic analysis.");
	QuasiIntegrator::preRun();
}

void ICKineIntegrator::firstStep()
{
	assert(false);
}

void ICKineIntegrator::subsequentSteps()
{
	assert(false);
}

void MbD::ICKineIntegrator::nextStep()
{
	assert(false);
}
