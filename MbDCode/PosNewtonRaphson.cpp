#include <sstream> 

#include "PosNewtonRaphson.h"
#include "SystemSolver.h"
#include "SimulationStoppingError.h"

using namespace MbD;

void MbD::PosNewtonRaphson::preRun()
{
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->prePosIC(); });
}

void MbD::PosNewtonRaphson::incrementIterNo()
{
	if (iterNo >= iterMax)
	{
		std::stringstream ss;
		ss << "MbD: No convergence after " << iterNo << " iterations.";
		auto str = ss.str();
		system->logString(str);
		ss.str("");
		ss << "MbD: A geometrically incompatible system has been specified.";
		str = ss.str();
		system->logString(str);
		ss.str("");
		ss << "MbD: Or the system parts are distributed too far apart from the assembled positions.";
		str = ss.str();
		system->logString(str);

		throw SimulationStoppingError("");
	}
	
	iterNo++;
}

void MbD::PosNewtonRaphson::askSystemToUpdate()
{
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->postPosICIteration(); });
}

void MbD::PosNewtonRaphson::postRun()
{
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->postPosIC(); });
}
