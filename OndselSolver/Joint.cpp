/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <algorithm>
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

Joint::Joint(const std::string& str) : ConstraintSet(str) {

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

void Joint::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillRedundantConstraints(con, redunConstraints); });
}

void Joint::removeRedundantConstraints(std::shared_ptr<std::vector<size_t>> redundantEqnNos)
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
