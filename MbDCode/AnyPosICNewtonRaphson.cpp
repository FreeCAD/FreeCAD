#include "AnyPosICNewtonRaphson.h"

void MbD::AnyPosICNewtonRaphson::initialize()
{
	NewtonRaphson::initialize();
	nSingularMatrixError = 0;
}
