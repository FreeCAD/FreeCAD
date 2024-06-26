/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <iostream>
#include <sstream> 

#include "PosNewtonRaphson.h"
#include "SystemSolver.h"
#include "SimulationStoppingError.h"

using namespace MbD;

void PosNewtonRaphson::preRun()
{
	system->partsJointsMotionsLimitsDo([&](std::shared_ptr<Item> item) { item->prePosIC(); });
}

void PosNewtonRaphson::incrementIterNo()
{
	iterNo++;
	if (iterNo > iterMax)
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

		throw SimulationStoppingError("iterNo > iterMax");
	}	
}

void PosNewtonRaphson::askSystemToUpdate()
{
	system->partsJointsMotionsLimitsDo([&](std::shared_ptr<Item> item) { item->postPosICIteration(); });
}

void PosNewtonRaphson::postRun()
{
	system->partsJointsMotionsLimitsDo([&](std::shared_ptr<Item> item) { item->postPosIC(); });
}
