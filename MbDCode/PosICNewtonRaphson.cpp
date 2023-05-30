#include <assert.h>
#include <exception>

#include "PosICNewtonRaphson.h"
#include "SingularMatrixError.h"
#include "SystemSolver.h"
#include "Part.h"
#include "Constraint.h"

using namespace MbD;

void MbD::PosICNewtonRaphson::run()
{
	while (true) {
		try {
			VectorNewtonRaphson::run();
			break;
		}
		catch (SingularMatrixError ex) {
			auto redundantEqnNos = ex.getRedundantEqnNos();
			system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->removeRedundantConstraints(redundantEqnNos); });
			system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->constraintsReport(); });
			system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->setqsu(qsuOld); });
		}
	}
}

void MbD::PosICNewtonRaphson::preRun()
{
	std::string str("MbD: Assembling system. ");
	system->logString(str);
	PosNewtonRaphson::preRun();
}

void MbD::PosICNewtonRaphson::assignEquationNumbers()
{
	auto parts = system->parts();
	//auto contactEndFrames = system->contactEndFrames();
	//auto uHolders = system->uHolders();
	auto essentialConstraints = system->essentialConstraints2();
	auto displacementConstraints = system->displacementConstraints();
	auto perpendicularConstraints = system->perpendicularConstraints2();
	auto eqnNo = 0;
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
	//	eqnNo = eqnNo + 1;
	//}
	//nqsu = eqnNo - 1;
	for (auto& con : *essentialConstraints) {
		con->iG = eqnNo;
		eqnNo = eqnNo + 1;
	}
	auto lastEssenConEqnNo = eqnNo - 1;
	for (auto& con : *displacementConstraints) {
		con->iG = eqnNo;
		eqnNo = eqnNo + 1;
	}
	auto lastDispConEqnNo = eqnNo - 1;
	for (auto& con : *perpendicularConstraints) {
		con->iG = eqnNo;
		eqnNo = eqnNo + 1;
	}
	n = eqnNo;
	auto limits = { lastEssenConEqnNo, lastDispConEqnNo, n };
	pivotRowLimits = std::make_shared<Array<int>>(limits);
}
