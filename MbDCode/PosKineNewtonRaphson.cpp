#include "PosKineNewtonRaphson.h"
#include "SystemSolver.h"

using namespace MbD;

void MbD::PosKineNewtonRaphson::initializeGlobally()
{
	SystemNewtonRaphson::initializeGlobally();
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->fillqsu(x); });
	iterMax = system->iterMaxPosKine;
	dxTol = system->errorTolPosKine;
}
