#include "PosICDragLimitNewtonRaphson.h"
#include "SystemSolver.h"
#include "SimulationStoppingError.h"
#include "Part.h"
#include "Constraint.h"

using namespace MbD;

std::shared_ptr<PosICDragLimitNewtonRaphson> MbD::PosICDragLimitNewtonRaphson::With()
{
	auto newtonRaphson = std::make_shared<PosICDragLimitNewtonRaphson>();
	newtonRaphson->initialize();
	return newtonRaphson;
}

void MbD::PosICDragLimitNewtonRaphson::preRun()
{
	std::string str("MbD: Assembling system with limits. ");
	system->logString(str);
	system->partsJointsMotionsLimitsDo([&](std::shared_ptr<Item> item) { item->prePosIC(); });

}

void MbD::PosICDragLimitNewtonRaphson::initializeGlobally()
{
	AnyPosICNewtonRaphson::initializeGlobally();
	iterMax = system->iterMaxPosKine;
	dxTol = system->errorTolPosKine;
}

void MbD::PosICDragLimitNewtonRaphson::setdragParts(std::shared_ptr<std::vector<std::shared_ptr<Part>>> dragParts)
{
	assert(false);
}

void MbD::PosICDragLimitNewtonRaphson::run()
{
	preRun();
	system->deactivateLimits();
	if (system->limitsSatisfied()) return;
	for (auto& limit : *system->limits()) {
		limit->activate();
		preRun();
		initializeLocally();
		initializeGlobally();
		iterate();
		postRun();
		system->deactivateLimits();
		if (system->limitsSatisfied()) return;
	}
	throw SimulationStoppingError("Limits cannot be satisfiled.");
}
