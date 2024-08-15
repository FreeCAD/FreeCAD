/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <memory>

#include "QuasiIntegrator.h"
#include "Item.h"
#include "SystemSolver.h"
#include "CREATE.h"
#include "BasicQuasiIntegrator.h"
#include "SingularMatrixError.h"
#include "SimulationStoppingError.h"
#include "TooSmallStepSizeError.h"
#include "TooManyTriesError.h"
#include "SingularMatrixError.h"
#include "DiscontinuityError.h"

using namespace MbD;

void QuasiIntegrator::preRun()
{
	system->partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->preDyn(); });
}

void QuasiIntegrator::initialize()
{
	Solver::initialize();
	integrator = CREATE<BasicQuasiIntegrator>::With();
	integrator->setSystem(this);
}

void QuasiIntegrator::run()
{
	try {
		try {
			try {
				IntegratorInterface::run();
			}
			catch (const SingularMatrixError& ex) {
				std::stringstream ss;
				ss << "MbD: Solver has encountered a singular matrix." << std::endl;
				ss << "MbD: Check to see if a massless or a very low mass part is under constrained." << std::endl;
				ss << "MbD: Check to see if the system is in a locked position." << std::endl;
				ss << "MbD: Check to see if the error tolerance is too demanding." << std::endl;
				ss << "MbD: Check to see if a curve-curve is about to have multiple contact points." << std::endl;
				auto str = ss.str();
				this->logString(str);
				throw SimulationStoppingError("singular matrix");
			}
		}
		catch (const TooSmallStepSizeError& ex) {
			std::stringstream ss;
			ss << "MbD: Step size is prevented from going below the user specified minimum." << std::endl;
			ss << "MbD: Check to see if the system is in a locked position." << std::endl;
			ss << "MbD: Check to see if a curve-curve is about to have multiple contact points." << std::endl;
			ss << "MbD: If they are not, lower the permitted minimum step size." << std::endl;
			auto str = ss.str();
			this->logString(str);
			throw SimulationStoppingError("stepSize < stepSizeMin");
		}
	}
	catch (const TooManyTriesError& ex) {
		std::stringstream ss;
		ss << "MbD: Check to see if the error tolerance is too demanding." << std::endl;
		auto str = ss.str();
		this->logString(str);
		throw SimulationStoppingError("iTry > iTryMax");
	}

}

void QuasiIntegrator::preFirstStep()
{
	system->partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->preDynFirstStep(); });
}

void QuasiIntegrator::postFirstStep()
{
	system->partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->postDynFirstStep(); });
	if (integrator->istep > 0) {
		//"Noise make checking at the start unreliable."
		this->checkForDiscontinuity();
	}
	this->checkForOutputThrough(integrator->t);
}

void QuasiIntegrator::preStep()
{
	system->partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->preDynStep(); });
}

void QuasiIntegrator::checkForDiscontinuity()
{
	//"Check for discontinuity in (tpast,t] or [t,tpast) if integrating 
	//backward."

	auto t = integrator->t;
	auto tprevious = integrator->tprevious();
	auto epsilon = std::numeric_limits<double>::epsilon();
	double tstartNew;
	if (direction == 0) {
		tstartNew = epsilon;
	}
	else {
		epsilon = std::abs(t) * epsilon;
		tstartNew = ((direction * t) + epsilon) / direction;
	}
	system->partsJointsMotionsLimitsForcesTorquesDo([&](std::shared_ptr<Item> item) { tstartNew = item->checkForDynDiscontinuityBetweenand(tprevious, tstartNew); });
	if ((direction * tstartNew) > (direction * t)) {
		//"No discontinuity in step"
			return;
	}
	else {
		this->checkForOutputThrough(tstartNew);
			this->interpolateAt(tstartNew);
			system->tstartPastsAddFirst(tstart);
			system->tstart = tstartNew;
			system->toutFirst = tout;
			auto discontinuityTypes = std::make_shared<std::vector<DiscontinuityType>>();
			system->partsJointsMotionsLimitsForcesTorquesDo([&](std::shared_ptr<Item> item) { item->discontinuityAtaddTypeTo(tstartNew, discontinuityTypes); });
			this->throwDiscontinuityError("", discontinuityTypes);
	}
}

double QuasiIntegrator::suggestSmallerOrAcceptFirstStepSize(double hnew)
{
	auto hnew2 = hnew;
	system->partsJointsMotionsLimitsForcesTorquesDo([&](std::shared_ptr<Item> item) { hnew2 = item->suggestSmallerOrAcceptDynFirstStepSize(hnew2); });
	if (hnew2 > hmax) {
		hnew2 = hmax;
		std::string str = "StM: Step size is at user specified maximum.";
		this->logString(str);
	}
	if (hnew2 < hmin) {
		std::stringstream ss;
		ss << "StM: Step size " << hnew2 << " < " << hmin << " user specified minimum.";
		auto str = ss.str();
		this->logString(str);
		throw TooSmallStepSizeError("");
	}
	return hnew2;
}

double QuasiIntegrator::suggestSmallerOrAcceptStepSize(double hnew)
{
	auto hnew2 = hnew;
	system->partsJointsMotionsLimitsForcesTorquesDo([&](std::shared_ptr<Item> item) { hnew2 = item->suggestSmallerOrAcceptDynStepSize(hnew2); });
	if (hnew2 > hmax) {
		hnew2 = hmax;
		this->logString("StM: Step size is at user specified maximum.");
	}
	if (hnew2 < hmin) {
		std::stringstream ss;
		ss << "StM: Step size " << hnew2 << " < " << hmin << " user specified minimum.";
		auto str = ss.str();
		system->logString(str);
		throw TooSmallStepSizeError("");
	}
	return hnew2;
}

void QuasiIntegrator::incrementTime(double tnew)
{
	system->partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->storeDynState(); });
	IntegratorInterface::incrementTime(tnew);
}

void QuasiIntegrator::throwDiscontinuityError(const std::string& chars, std::shared_ptr<std::vector<DiscontinuityType>> discontinuityTypes)
{
	throw DiscontinuityError(chars, discontinuityTypes);
}

void QuasiIntegrator::checkForOutputThrough(double t)
{
	//"Kinematic analysis is done at every tout."
	if (direction * t <= (direction * (tend + (0.1 * direction * hout)))) {
		if (std::abs(tout - t) < 1.0e-12) {
			system->output();
			tout += direction * hout;
		}
	}
	else {
		integrator->_continue = false;
	}
}

void QuasiIntegrator::interpolateAt(double tArg)
{
	//"Interpolate for system state at tArg and leave system in that state."
	system->time(tArg);
	this->runInitialConditionTypeSolution();
}

void QuasiIntegrator::postStep()
{
	system->partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->postDynStep(); });

	if (integrator->istep > 0) {
		//"Noise make checking at the start unreliable."
		this->checkForDiscontinuity();
	}
	this->checkForOutputThrough(integrator->t);
}

void QuasiIntegrator::postRun()
{
	system->partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->postDyn(); });
}
