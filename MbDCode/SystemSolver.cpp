#include "SystemSolver.h"

using namespace MbD;

void SystemSolver::initializeLocally()
{
	setsOfRedundantConstraints = std::make_unique<std::vector<std::vector<std::shared_ptr<Constraint>>>>();
	direction = (tstart < tend) ? 1.0 : -1.0;
	toutFirst = tstart + (direction * hout);
}

void SystemSolver::initializeGlobally()
{
}

void SystemSolver::runAllIC()
{
}

void SystemSolver::runBasicKinematic()
{
}
