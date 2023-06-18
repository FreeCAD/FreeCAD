#include "ICKineIntegrator.h"
#include "SystemSolver.h"

void MbD::ICKineIntegrator::runInitialConditionTypeSolution()
{
	system->runPosICKine();
	system->runVelICKine();
	system->runAccICKine();
}
