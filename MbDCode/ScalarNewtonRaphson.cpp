#include "ScalarNewtonRaphson.h"
#include "SystemSolver.h"

using namespace MbD;

void MbD::ScalarNewtonRaphson::initializeGlobally()
{
	assert(false);
	//x = system->x;
}

void MbD::ScalarNewtonRaphson::calcyNorm()
{
	yNorm = 0.5 * y * y;
}

void MbD::ScalarNewtonRaphson::solveEquations()
{
	dx = -y / pypx;
}

void MbD::ScalarNewtonRaphson::updatexold()
{
	xold = x;
}

void MbD::ScalarNewtonRaphson::calcdxNorm()
{
	dxNorm = std::abs(dx);
}

void MbD::ScalarNewtonRaphson::xEqualxoldPlusdx()
{
	x = xold + dx;
}
