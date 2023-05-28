#include "PosNewtonRaphson.h"
#include "SystemSolver.h"

using namespace MbD;

void MbD::PosNewtonRaphson::preRun()
{
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->prePosIC(); });
}
