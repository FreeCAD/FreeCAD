#include <vector>
#include <set>
#include <algorithm>

#include "SystemSolver.h"
#include "System.h"
#include "NewtonRaphson.h"
#include "PosICNewtonRaphson.h"
#include "CREATE.h"
#include "RedundantConstraint.h"
#include "NotKinematicError.h"
#include "ICKineIntegrator.h"
#include "KineIntegrator.h"
#include "DiscontinuityError.h"
#include "PosICKineNewtonRaphson.h"
#include "PosKineNewtonRaphson.h"
#include "VelICSolver.h"
#include "AccICNewtonRaphson.h"
#include "VelKineSolver.h"
#include "AccKineNewtonRaphson.h"

using namespace MbD;

//class PosICNewtonRaphson;

void SystemSolver::setSystem(Solver* sys)
{
	//Do not use
	assert(false);
}

void SystemSolver::initialize()
{
	tstartPasts = std::make_shared<std::vector<double>>();
}

void SystemSolver::initializeLocally()
{
	setsOfRedundantConstraints = std::make_shared<std::vector<std::shared_ptr<std::set<std::string>>>>();
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

void SystemSolver::runPosIC()
{
	icTypeSolver = CREATE<PosICNewtonRaphson>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void SystemSolver::runVelIC()
{
	icTypeSolver = CREATE<VelICSolver>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void SystemSolver::runAccIC()
{
	icTypeSolver = CREATE<AccICNewtonRaphson>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

bool SystemSolver::needToRedoPosIC()
{
	auto allRedunCons = this->allRedundantConstraints();
	auto newSet = std::make_shared<std::set<std::string>>();
	for (auto& con : *allRedunCons) {
		auto aaa = std::static_pointer_cast<RedundantConstraint>(con);
		auto& bbb = aaa->constraint->name;
		newSet->insert(bbb);
	}
	//std::transform(allRedunCons->begin(), allRedunCons->end(), newSet->begin(), [](auto con) {
	//	return std::static_pointer_cast<RedundantConstraint>(con)->constraint->name;
	//	});
	if (newSet->empty()) return false;
	auto itr = std::find_if(setsOfRedundantConstraints->begin(), setsOfRedundantConstraints->end(), [&](auto& set) {
		for (auto& name : *set) {
			if (newSet->find(name) == newSet->end()) return false;
		}
		return true;
		});
	if (itr != setsOfRedundantConstraints->end()) {
		//"Same set of redundant constraints found."
		setsOfRedundantConstraints->push_back(newSet);
		return false;
	}
	if (setsOfRedundantConstraints->size() >= 2) {
		auto it = std::find_if(setsOfRedundantConstraints->begin(), setsOfRedundantConstraints->end(), [&](auto set) {
			return set->size() == newSet->size();
			});
		if (it != setsOfRedundantConstraints->end()) {
			//"Equal number of redundant constraints found."
			setsOfRedundantConstraints->push_back(newSet);
			return false;
		}
	}
	setsOfRedundantConstraints->push_back(newSet);
	this->partsJointsMotionsDo([](auto item) { item->reactivateRedundantConstraints(); });
	return true;
}

void SystemSolver::preCollision()
{
}

void SystemSolver::runCollisionDerivativeIC()
{
}

void SystemSolver::runBasicCollision()
{
}

void SystemSolver::runBasicKinematic()
{
	try {
		basicIntegrator = CREATE<KineIntegrator>::With();
		basicIntegrator->setSystem(this);
		basicIntegrator->run();
	}
	catch (NotKinematicError ex) {
		this->runQuasiKinematic();
	}
}

void SystemSolver::runQuasiKinematic()
{
	try {
		basicIntegrator = CREATE<ICKineIntegrator>::With();
		basicIntegrator->setSystem(this);
		basicIntegrator->run();
	}
	catch (DiscontinuityError ex) {
		this->discontinuityBlock();
	}
}

void SystemSolver::runPosKine()
{
	icTypeSolver = CREATE<PosKineNewtonRaphson>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void SystemSolver::runVelKine()
{
	icTypeSolver = CREATE<VelKineSolver>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void SystemSolver::runAccKine()
{
	icTypeSolver = CREATE<AccKineNewtonRaphson>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void SystemSolver::runPosICKine()
{
	icTypeSolver = CREATE<PosICKineNewtonRaphson>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void SystemSolver::runVelICKine()
{
	assert(false);
	//icTypeSolver = CREATE<VelICKineSolver>::With();
	//icTypeSolver->setSystem(this);
	//icTypeSolver->run();
}

void SystemSolver::runAccICKine()
{
	assert(false);
	//icTypeSolver = CREATE<AccICKineNewtonRaphson>::With();
	//icTypeSolver->setSystem(this);
	//icTypeSolver->run();
}

void SystemSolver::partsJointsMotionsDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	system->partsJointsMotionsDo(f);
}

void SystemSolver::logString(std::string& str)
{
	system->logString(str);
}

std::shared_ptr<std::vector<std::shared_ptr<Part>>> SystemSolver::parts()
{
	return system->parts;
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> SystemSolver::essentialConstraints2()
{
	return system->essentialConstraints2();
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> SystemSolver::displacementConstraints()
{
	return system->displacementConstraints();
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> SystemSolver::perpendicularConstraints()
{
	return system->perpendicularConstraints();
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> SystemSolver::allRedundantConstraints()
{
	return system->allRedundantConstraints();
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> SystemSolver::allConstraints()
{
	return system->allConstraints();
}

void SystemSolver::postNewtonRaphson()
{
	assert(false);
}

void SystemSolver::partsJointsMotionsForcesTorquesDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	system->partsJointsMotionsForcesTorquesDo(f);
}

void SystemSolver::discontinuityBlock()
{
	assert(false);
}

double SystemSolver::startTime()
{
	return tstart;
}

double SystemSolver::outputStepSize()
{
	return hout;
}

double SystemSolver::maxStepSize()
{
	return hmax;
}

double SystemSolver::minStepSize()
{
	return hmin;
}

double SystemSolver::firstOutputTime()
{
	return toutFirst;
}

double SystemSolver::endTime()
{
	return tend;
}

void SystemSolver::settime(double tnew)
{
	system->mbdTimeValue(tnew);
}

void SystemSolver::tstartPastsAddFirst(double tstartPast)
{
	tstartPasts->insert(tstartPasts->begin(), tstartPast);
}

void SystemSolver::output()
{
	system->outputFor(DYNAMIC);
}

void SystemSolver::time(double t)
{
	system->mbdTimeValue(t);
}
