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
#include "ForceTorqueData.h"
#include "System.h"

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

void Joint::constraintsDo(const std::function<void(std::shared_ptr<Constraint>)>& f)
{
	std::for_each(constraints->begin(), constraints->end(), f);
}

void Joint::postInput()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postInput(); });

}

void Joint::addConstraint(std::shared_ptr<Constraint> con)
{
	con->owner = this;
	constraints->push_back(con);
}

FColDsptr MbD::Joint::aFIeJtIe()
{
	//"aFIeJtIe is joint force on end frame Ie expresses in Ie components."
	auto frmIqc = std::dynamic_pointer_cast<EndFrameqc>(frmI);
	return frmIqc->aAeO()->timesFullColumn(this->aFIeJtO());
}

FColDsptr MbD::Joint::aFIeJtO()
{
	//"aFIeJtO is joint force on end frame Ie expresses in O components."
	auto aFIeJtO = std::make_shared <FullColumn<double>>(3);
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->addToJointForceI(aFIeJtO); });
	return aFIeJtO;
}

void Joint::prePosIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->prePosIC(); });
}

void Joint::prePosKine()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->prePosKine(); });
}

void Joint::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillEssenConstraints(con, essenConstraints); });
}

void Joint::fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillDispConstraints(con, dispConstraints); });
}

void Joint::fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPerpenConstraints(con, perpenConstraints); });
}

void Joint::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillRedundantConstraints(con, redunConstraints); });
}

void Joint::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillConstraints(con, allConstraints); });
}

void Joint::fillqsulam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillqsulam(col); });
}

void Joint::fillqsudot(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillqsudot(col); });
}

void Joint::fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
}

void Joint::useEquationNumbers()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->useEquationNumbers(); });
}

void MbD::Joint::submitToSystem(std::shared_ptr<Item> self)
{
	root()->jointsMotions->push_back(std::static_pointer_cast<Joint>(self));
}

void Joint::setqsulam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->setqsulam(col); });
}

void Joint::setqsudotlam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->setqsudotlam(col); });
}

void Joint::postPosICIteration()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postPosICIteration(); });
}

void Joint::fillPosICError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPosICError(col); });
}

void Joint::fillPosICJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPosICJacob(mat); });
}

void Joint::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos)
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

void Joint::reactivateRedundantConstraints()
{
	for (size_t i = 0; i < constraints->size(); i++)
	{
		auto& con = constraints->at(i);
		if (con->isRedundant()) {
			constraints->at(i) = std::static_pointer_cast<RedundantConstraint>(con)->constraint;
		}
	}
}

void Joint::constraintsReport()
{
	auto redunCons = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	constraintsDo([&](std::shared_ptr<Constraint> con) {
		if (con->isRedundant()) {
			redunCons->push_back(con);
		}
		});
	if (redunCons->size() > 0) {
		std::string str = "MbD: " + this->classname() + std::string(" ") + this->name + " has the following constraint(s) removed: ";
		this->logString(str);
		std::for_each(redunCons->begin(), redunCons->end(), [&](auto& con) {
			str = "MbD: " + std::string("    ") + con->classname();
			this->logString(str);
			});
	}
}

void Joint::postPosIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postPosIC(); });
}

void Joint::preDyn()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->preDyn(); });
}

void Joint::fillPosKineError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPosKineError(col); });
}

void Joint::fillPosKineJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> constraint) { constraint->fillPosKineJacob(mat); });
}

void MbD::Joint::fillqsuddotlam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> constraint) { constraint->fillqsuddotlam(col); });
}

void Joint::preVelIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->preVelIC(); });
}

void Joint::fillVelICError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillVelICError(col); });
}

void Joint::fillVelICJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> constraint) { constraint->fillVelICJacob(mat); });
}

void Joint::preAccIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->preAccIC(); });
}

void Joint::fillAccICIterError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillAccICIterError(col); });
}

void Joint::fillAccICIterJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillAccICIterJacob(mat); });
}

void Joint::setqsuddotlam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->setqsuddotlam(col); });
}

std::shared_ptr<StateData> Joint::stateData()
{
	//"
	//MbD returns aFIeO and aTIeO.
	//GEO needs aFImO and aTImO.
	//For Motion rImIeO is not zero and is changing.
	//aFImO : = aFIeO.
	//aTImO : = aTIeO + (rImIeO cross : aFIeO).
	//"

	auto answer = std::make_shared<ForceTorqueData>();
	auto aFIeO = this->aFX();
	auto aTIeO = this->aTX();
	auto rImIeO = this->frmI->rmeO();
	answer->aFIO = aFIeO;
	answer->aTIO = aTIeO->plusFullColumn(rImIeO->cross(aFIeO));
	return answer;
}

FColDsptr Joint::aFX()
{
	return this->jointForceI();
}

FColDsptr MbD::Joint::aTIeJtIe()
{
	//"aTIeJtIe is torque on part containing end frame Ie expressed in Ie components."
	return frmI->aAeO()->timesFullColumn(this->aTIeJtO());
}

FColDsptr MbD::Joint::aTIeJtO()
{
	//"aTIeJtO is torque on part containing end frame Ie expressed in O components."
	auto aTIeJtO = std::make_shared <FullColumn<double>>(3);
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->addToJointTorqueI(aTIeJtO); });
	return aTIeJtO;
}

FColDsptr Joint::jointForceI()
{
	//"jointForceI is force on MbD marker I."
	auto jointForce = std::make_shared <FullColumn<double>>(3);
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->addToJointForceI(jointForce); });
	return jointForce;
}

FColDsptr Joint::aTX()
{
	return this->jointTorqueI();
}

FColDsptr Joint::jointTorqueI()
{
	//"jointTorqueI is torque on MbD marker I."
	auto jointTorque = std::make_shared <FullColumn<double>>(3);
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->addToJointTorqueI(jointTorque); });
	return jointTorque;
}

void Joint::postDynStep()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postDynStep(); });
}
