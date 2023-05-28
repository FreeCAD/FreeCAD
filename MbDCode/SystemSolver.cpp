#include <vector>

#include "SystemSolver.h"
#include "NewtonRaphson.h"
#include "PosICNewtonRaphson.h"

using namespace MbD;

//class PosICNewtonRaphson;

std::shared_ptr<SystemSolver> MbD::SystemSolver::Create(System* x)
{
	auto item = std::make_shared<SystemSolver>(x);
	item->initialize();
	return item;
}

void MbD::SystemSolver::initialize()
{
	tstartPasts = std::make_shared<std::vector<double>>();
}

void SystemSolver::initializeLocally()
{
	setsOfRedundantConstraints = std::make_shared<std::vector<std::vector<std::shared_ptr<Constraint>>>>();
	direction = (tstart < tend) ? 1.0 : -1.0;
	toutFirst = tstart + (direction * hout);
}

void SystemSolver::initializeGlobally()
{
}

void SystemSolver::runAllIC()
{
	while (true)
	{
		initializeLocally();
		initializeGlobally();
		runPosIC();
		while (needToRedoPosIC())
		{
			runPosIC();
		}
		runVelIC();
		runAccIC();
		auto discontinuities = system->discontinuitiesAtIC();
		if (discontinuities->size() == 0) break;
		if (std::find(discontinuities->begin(), discontinuities->end(), "REBOUND") != discontinuities->end())
		{
			preCollision();
			runCollisionDerivativeIC();
			runBasicCollision();
		}
	}
}

void MbD::SystemSolver::runPosIC()
{
	icTypeSolver = std::make_shared<PosICNewtonRaphson>();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void MbD::SystemSolver::runVelIC()
{
}

void MbD::SystemSolver::runAccIC()
{
}

bool MbD::SystemSolver::needToRedoPosIC()
{
	return false;
}

void MbD::SystemSolver::preCollision()
{
}

void MbD::SystemSolver::runCollisionDerivativeIC()
{
}

void MbD::SystemSolver::runBasicCollision()
{
}

void SystemSolver::runBasicKinematic()
{
}

void MbD::SystemSolver::partsJointsMotionsDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	system->partsJointsMotionsDo(f);
}
