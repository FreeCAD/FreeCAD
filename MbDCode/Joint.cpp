#include<algorithm>

#include "Joint.h"
#include "Constraint.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"

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
	constraintsDo([](const auto& constraint) { constraint->initializeLocally(); });
}

void Joint::initializeGlobally()
{
	constraintsDo([](const auto& constraint) { constraint->initializeGlobally(); });
}

void MbD::Joint::constraintsDo(const std::function<void(std::shared_ptr<Constraint>)>& f)
{
	std::for_each(constraints->begin(), constraints->end(), f);
}

void MbD::Joint::postInput()
{
	constraintsDo([](const auto& constraint) { constraint->postInput(); });

}

void MbD::Joint::addConstraint(std::shared_ptr<Constraint> con)
{
	con->setOwner(this);
	constraints->push_back(con);
}

void MbD::Joint::prePosIC()
{
	constraintsDo([](const auto& constraint) { constraint->prePosIC(); });
}

void MbD::Joint::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	constraintsDo([&](const auto& con) { con->fillEssenConstraints(con, essenConstraints); });
}

void MbD::Joint::fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
	constraintsDo([&](const auto& con) { con->fillDispConstraints(con, dispConstraints); });
}

void MbD::Joint::fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
	constraintsDo([&](const auto& con) { con->fillPerpenConstraints(con, perpenConstraints); });
}
