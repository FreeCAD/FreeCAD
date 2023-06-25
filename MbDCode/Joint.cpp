#include<algorithm>
#include <memory>
#include <typeinfo>

#include "Joint.h"
#include "Constraint.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"
#include "CREATE.h"
#include "RedundantConstraint.h"
#include "MarkerFrame.h"

using namespace MbD;

Joint::Joint() {

}

Joint::Joint(const char* str) : Item(str) {

}

void Joint::initialize()
{
	constraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
}

void Joint::connectsItoJ(EndFrmcptr frmi, EndFrmcptr frmj)
{
	frmI = frmi;
	frmJ = frmj;
}

void Joint::initializeLocally()
{
	auto frmIqc = std::dynamic_pointer_cast<EndFrameqc>(frmI);
	if (frmIqc) {
		if (frmIqc->endFrameqct) {
			frmI = frmIqc->endFrameqct;
		}
	}
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->initializeLocally(); });
}

void Joint::initializeGlobally()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->initializeGlobally(); });
}

void MbD::Joint::constraintsDo(const std::function<void(std::shared_ptr<Constraint>)>& f)
{
	std::for_each(constraints->begin(), constraints->end(), f);
}

void MbD::Joint::postInput()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postInput(); });

}

void MbD::Joint::addConstraint(std::shared_ptr<Constraint> con)
{
	con->setOwner(this);
	constraints->push_back(con);
}

void MbD::Joint::prePosIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->prePosIC(); });
}

void MbD::Joint::prePosKine()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->prePosKine(); });
}

void MbD::Joint::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillEssenConstraints(con, essenConstraints); });
}

void MbD::Joint::fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillDispConstraints(con, dispConstraints); });
}

void MbD::Joint::fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPerpenConstraints(con, perpenConstraints); });
}

void MbD::Joint::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillRedundantConstraints(con, redunConstraints); });
}

void MbD::Joint::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillConstraints(con, allConstraints); });
}

void MbD::Joint::fillqsulam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillqsulam(col); });
}

void MbD::Joint::fillqsudot(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillqsudot(col); });
}

void MbD::Joint::fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
}

void MbD::Joint::useEquationNumbers()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->useEquationNumbers(); });
}

void MbD::Joint::setqsulam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->setqsulam(col); });
}

void MbD::Joint::setqsudotlam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->setqsudotlam(col); });
}

void MbD::Joint::postPosICIteration()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postPosICIteration(); });
}

void MbD::Joint::fillPosICError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPosICError(col); });
}

void MbD::Joint::fillPosICJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPosICJacob(mat); });
}

void MbD::Joint::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos)
{
	for (size_t i = 0; i < constraints->size(); i++)
	{
		auto& constraint = constraints->at(i);
		if (std::find(redundantEqnNos->begin(), redundantEqnNos->end(), constraint->iG) != redundantEqnNos->end()) {
			auto redunCon = CREATE<RedundantConstraint>::With();
			redunCon->constraint = constraint;
			constraints->at(i) = redunCon;
		}
	}
}

void MbD::Joint::reactivateRedundantConstraints()
{
	for (size_t i = 0; i < constraints->size(); i++)
	{
		auto& con = constraints->at(i);
		if (con->isRedundant()) {
			constraints->at(i) = std::static_pointer_cast<RedundantConstraint>(con)->constraint;
		}
	}
}

void MbD::Joint::constraintsReport()
{
	auto redunCons = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	constraintsDo([&](std::shared_ptr<Constraint> con) {
		if (con->isRedundant()) {
			redunCons->push_back(con);
		}
		});
	if (redunCons->size() > 0) {
		std::string str = "MbD: " + this->classname() + std::string(" ") + this->getName() + " has the following constraint(s) removed: ";
		this->logString(str);
		std::for_each(redunCons->begin(), redunCons->end(), [&](auto& con) {
			str = "MbD: " + std::string("    ") + con->classname();
			this->logString(str);
			});
	}
}

void MbD::Joint::postPosIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postPosIC(); });
}

void MbD::Joint::outputStates()
{
	Item::outputStates();
	std::stringstream ss;
	ss << "frmI = " << frmI->markerFrame->getName() << std::endl;
	ss << "frmJ = " << frmJ->markerFrame->getName() << std::endl;
	auto str = ss.str();
	this->logString(str);

	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->outputStates(); });
}

void MbD::Joint::preDyn()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->preDyn(); });
}

void MbD::Joint::fillPosKineError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPosKineError(col); });
}

void MbD::Joint::fillPosKineJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> constraint) { constraint->fillPosKineJacob(mat); });
}

void MbD::Joint::preVelIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->preVelIC(); });
}

void MbD::Joint::fillVelICError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillVelICError(col); });
}

void MbD::Joint::fillVelICJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> constraint) { constraint->fillVelICJacob(mat); });
}

void MbD::Joint::preAccIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->preAccIC(); });
}

void MbD::Joint::fillAccICIterError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillAccICIterError(col); });
}

void MbD::Joint::fillAccICIterJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillAccICIterJacob(mat); });
}

void MbD::Joint::setqsuddotlam(FColDsptr qsudotlam)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->setqsuddotlam(qsudotlam); });
}
