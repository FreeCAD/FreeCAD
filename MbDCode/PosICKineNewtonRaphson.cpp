#include "PosICKineNewtonRaphson.h"
#include "SystemSolver.h"

using namespace MbD;

void MbD::PosICKineNewtonRaphson::initializeGlobally()
{
	AnyPosICNewtonRaphson::initializeGlobally();
	iterMax = system->iterMaxPosKine;
	dxTol = system->errorTolPosKine;
}

void MbD::PosICKineNewtonRaphson::assignEquationNumbers()
{
}
