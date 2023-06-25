#include <vector>
#include <set>
#include <algorithm>

#include "SystemSolver.h"
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

using namespace MbD;

//class PosICNewtonRaphson;

void MbD::SystemSolver::setSystem(Solver* sys)
{
	//Do not use
	assert(false);
}

void MbD::SystemSolver::initialize()
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

void MbD::SystemSolver::runPosIC()
{
	icTypeSolver = CREATE<PosICNewtonRaphson>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void MbD::SystemSolver::runVelIC()
{
	icTypeSolver = CREATE<VelICSolver>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void MbD::SystemSolver::runAccIC()
{
	icTypeSolver = CREATE<AccICNewtonRaphson>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

bool MbD::SystemSolver::needToRedoPosIC()
{
	auto allRedunCons = this->allRedundantConstraints();
	auto newSet = std::make_shared<std::set<std::string>>();
	for (auto& con : *allRedunCons) {
		auto aaa = std::static_pointer_cast<RedundantConstraint>(con);
		auto& bbb = aaa->constraint->getName();
		newSet->insert(bbb);
	}
	//std::transform(allRedunCons->begin(), allRedunCons->end(), newSet->begin(), [](auto con) {
	//	return std::static_pointer_cast<RedundantConstraint>(con)->constraint->getName();
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
	try {
		basicIntegrator = CREATE<KineIntegrator>::With();
		basicIntegrator->setSystem(this);
		basicIntegrator->run();
	}
	catch (NotKinematicError ex) {
		this->runQuasiKinematic();
	}
}

void MbD::SystemSolver::runQuasiKinematic()
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

void MbD::SystemSolver::runPosKine()
{
	icTypeSolver = CREATE<PosKineNewtonRaphson>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void MbD::SystemSolver::runVelKine()
{
	assert(false);
	//icTypeSolver = CREATE<VelKineSolver>::With();
	//icTypeSolver->setSystem(this);
	//icTypeSolver->run();
}

void MbD::SystemSolver::runAccKine()
{
	assert(false);
	//icTypeSolver = CREATE<AccKineNewtonRaphson>::With();
	//icTypeSolver->setSystem(this);
	//icTypeSolver->run();
}

void MbD::SystemSolver::runPosICKine()
{
	icTypeSolver = CREATE<PosICKineNewtonRaphson>::With();
	icTypeSolver->setSystem(this);
	icTypeSolver->run();
}

void MbD::SystemSolver::runVelICKine()
{
	assert(false);
	//icTypeSolver = CREATE<VelICKineSolver>::With();
	//icTypeSolver->setSystem(this);
	//icTypeSolver->run();
}

void MbD::SystemSolver::runAccICKine()
{
	assert(false);
	//icTypeSolver = CREATE<AccICKineNewtonRaphson>::With();
	//icTypeSolver->setSystem(this);
	//icTypeSolver->run();
}

void MbD::SystemSolver::partsJointsMotionsDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	system->partsJointsMotionsDo(f);
}

void MbD::SystemSolver::logString(std::string& str)
{
	system->logString(str);
}

std::shared_ptr<std::vector<std::shared_ptr<Part>>> MbD::SystemSolver::parts()
{
	return system->parts;
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> MbD::SystemSolver::essentialConstraints2()
{
	return system->essentialConstraints2();
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> MbD::SystemSolver::displacementConstraints()
{
	return system->displacementConstraints();
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> MbD::SystemSolver::perpendicularConstraints()
{
	return system->perpendicularConstraints();
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> MbD::SystemSolver::allRedundantConstraints()
{
	return system->allRedundantConstraints();
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> MbD::SystemSolver::allConstraints()
{
	return system->allConstraints();
}

void MbD::SystemSolver::postNewtonRaphson()
{
	assert(false);
}

void MbD::SystemSolver::partsJointsMotionsForcesTorquesDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	system->partsJointsMotionsForcesTorquesDo(f);
}

void MbD::SystemSolver::discontinuityBlock()
{
	assert(false);
}

double MbD::SystemSolver::startTime()
{
	return tstart;
}

double MbD::SystemSolver::outputStepSize()
{
	return hout;
}

double MbD::SystemSolver::maxStepSize()
{
	return hmax;
}

double MbD::SystemSolver::minStepSize()
{
	return hmin;
}

double MbD::SystemSolver::firstOutputTime()
{
	return toutFirst;
}

double MbD::SystemSolver::endTime()
{
	return tend;
}

void MbD::SystemSolver::settime(double tnew)
{
	system->mbdTimeValue(tnew);
}
