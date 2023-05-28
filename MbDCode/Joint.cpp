#include<algorithm>

#include "Joint.h"
#include "Constraint.h"

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
	std::for_each(constraints->begin(), constraints->end(), [](const auto& constraint) { constraint->initializeLocally(); });
}

void Joint::initializeGlobally()
{
	std::for_each(constraints->begin(), constraints->end(), [](const auto& constraint) { constraint->initializeGlobally(); });
}

void MbD::Joint::postInput()
{
}

void MbD::Joint::addConstraint(std::shared_ptr<Constraint> con)
{
	con->setOwner(this);
	constraints->push_back(con);
}
