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

using namespace MbD;

void MbD::QuasiIntegrator::preRun()
{
	system->partsJointsMotionsForcesTorquesDo([](std::shared_ptr<Item> item) { item->preDyn(); });
}

void MbD::QuasiIntegrator::initialize()
{
	Solver::initialize();
	integrator = CREATE<BasicQuasiIntegrator>::With();
	integrator->setSystem(this);
}

void MbD::QuasiIntegrator::run()
{
	try {
		try {
			try {
				IntegratorInterface::run();
			}
			catch (SingularMatrixError ex) {
				std::stringstream ss;
				ss << "MbD: Solver has encountered a singular matrix." << std::endl;
				ss << "MbD: Check to see if a massless or a very low mass part is under constrained." << std::endl;
				ss << "MbD: Check to see if the system is in a locked position." << std::endl;
				ss << "MbD: Check to see if the error tolerance is too demanding." << std::endl;
				ss << "MbD: Check to see if a curve-curve is about to have multiple contact points." << std::endl;
				auto str = ss.str();
				this->logString(str);
				throw SimulationStoppingError("");
			}
		}
		catch (TooSmallStepSizeError ex) {
			std::stringstream ss;
			ss << "MbD: Step size is prevented from going below the user specified minimum." << std::endl;
			ss << "MbD: Check to see if the system is in a locked position." << std::endl;
			ss << "MbD: Check to see if a curve-curve is about to have multiple contact points." << std::endl;
			ss << "MbD: If they are not, lower the permitted minimum step size." << std::endl;
			auto str = ss.str();
			this->logString(str);
			throw SimulationStoppingError("");
		}
	}
	catch (TooManyTriesError ex) {
		std::stringstream ss;
		ss << "MbD: Check to see if the error tolerance is too demanding." << std::endl;
		auto str = ss.str();
		this->logString(str);
		throw SimulationStoppingError("");
	}

}

void MbD::QuasiIntegrator::preFirstStep()
{
	system->partsJointsMotionsForcesTorquesDo([](std::shared_ptr<Item> item) { item->preDynFirstStep(); });
}

void MbD::QuasiIntegrator::preStep()
{
	system->partsJointsMotionsForcesTorquesDo([](std::shared_ptr<Item> item) { item->preDynStep(); });
}

double MbD::QuasiIntegrator::suggestSmallerOrAcceptFirstStepSize(double hnew)
{
	auto hnew2 = hnew;
	system->partsJointsMotionsForcesTorquesDo([&](std::shared_ptr<Item> item) { hnew2 = item->suggestSmallerOrAcceptDynFirstStepSize(hnew2); });
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

void MbD::QuasiIntegrator::incrementTime(double tnew)
{
	system->partsJointsMotionsForcesTorquesDo([](std::shared_ptr<Item> item) { item->storeDynState(); });
	IntegratorInterface::incrementTime(tnew);
}
