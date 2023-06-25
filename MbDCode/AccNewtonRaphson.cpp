#include "AccNewtonRaphson.h"
#include "SystemSolver.h"
#include "Part.h"
#include "Constraint.h"
#include "SimulationStoppingError.h"
#include <iostream>

void MbD::AccNewtonRaphson::askSystemToUpdate()
{
	system->partsJointsMotionsForcesTorquesDo([&](std::shared_ptr<Item> item) { item->postAccICIteration(); });
}

void MbD::AccNewtonRaphson::assignEquationNumbers()
{
	auto parts = system->parts();
	//auto contactEndFrames = system->contactEndFrames();
	//auto uHolders = system->uHolders();
	auto constraints = system->allConstraints();
	int eqnNo = 0;
	for (auto& part : *parts) {
		part->iqX(eqnNo);
		eqnNo = eqnNo + 3;
		part->iqE(eqnNo);
		eqnNo = eqnNo + 4;
	}
	//for (auto& endFrm : *contactEndFrames) {
	//	endFrm->is(eqnNo);
	//	eqnNo = eqnNo + endFrm->sSize();
	//}
	//for (auto& uHolder : *uHolders) {
	//	uHolder->iu(eqnNo);
	//	eqnNo += 1;
	//}
	auto nEqns = eqnNo;	//C++ uses index 0.
	for (auto& con : *constraints) {
		con->iG = eqnNo;
		eqnNo += 1;
	}
	auto lastEqnNo = eqnNo - 1;
	nEqns = eqnNo;	//C++ uses index 0.
	n = nEqns;
}

void MbD::AccNewtonRaphson::fillPyPx()
{
	pypx->zeroSelf();
	system->partsJointsMotionsForcesTorquesDo([&](std::shared_ptr<Item> item) {
		item->fillAccICIterJacob(pypx);
		});
}

void MbD::AccNewtonRaphson::fillY()
{
	y->zeroSelf();
	system->partsJointsMotionsForcesTorquesDo([&](std::shared_ptr<Item> item) {
		item->fillAccICIterError(y);
		//std::cout << item->getName() << *y << std::endl;
		});
	//std::cout << *y << std::endl;
}

void MbD::AccNewtonRaphson::incrementIterNo()
{
	if (iterNo >= iterMax)
	{
		std::stringstream ss;
		ss << "MbD: No convergence after " << iterNo << " iterations.";
		auto str = ss.str();
		system->logString(str);
		ss.str("");
		ss << "A force function of joint reactions can cause this problem";
		str = ss.str();
		system->logString(str);
		ss.str("");
		ss << "if the function returns large values.";
		str = ss.str();
		system->logString(str);

		throw SimulationStoppingError("");
	}

	iterNo++;
}

void MbD::AccNewtonRaphson::initializeGlobally()
{
	SystemNewtonRaphson::initializeGlobally();
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->fillqsuddotlam(x); });
}

void MbD::AccNewtonRaphson::logSingularMatrixMessage()
{
	std::string str = "MbD: Some parts with zero masses or moment of inertias have infinite accelerations.";
	system->logString(str);
	str = "Add masses and inertias to or properly constrain those parts.";
	system->logString(str);
}

void MbD::AccNewtonRaphson::passRootToSystem()
{
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->setqsuddotlam(x); });
}

void MbD::AccNewtonRaphson::postRun()
{
	system->partsJointsMotionsForcesTorquesDo([&](std::shared_ptr<Item> item) { item->postAccIC(); });
}

void MbD::AccNewtonRaphson::preRun()
{
	system->partsJointsMotionsForcesTorquesDo([&](std::shared_ptr<Item> item) { item->preAccIC(); });
}
