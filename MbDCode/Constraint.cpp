#include "Constraint.h"
#include "enum.h"

using namespace MbD;

Constraint::Constraint()
{
}

Constraint::Constraint(const char* str) : Item(str)
{
}

void Constraint::initialize()
{
	iG = -1;
	aG = 0.0;
	lam = 0.0;
}

void MbD::Constraint::postInput()
{
	lam = 0.0;
	Item::postInput();
}

void Constraint::setOwner(Item* x)
{
	owner = x;
}

Item* Constraint::getOwner()
{
	return owner;
}

void MbD::Constraint::prePosIC()
{
	lam = 0.0;
	iG = -1;
	Item::prePosIC();
}

void MbD::Constraint::fillEssenConstraints(std::shared_ptr<Constraint>sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	if (this->type() == MbD::essential) {
		essenConstraints->push_back(sptr);
	}
}
void MbD::Constraint::fillDispConstraints(std::shared_ptr<Constraint>sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
	if (this->type() == MbD::displacement) {
		dispConstraints->push_back(sptr);
	}
}
void MbD::Constraint::fillPerpenConstraints(std::shared_ptr<Constraint>sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
	if (this->type() == MbD::perpendicular) {
		perpenConstraints->push_back(sptr);
	}
}

MbD::ConstraintType MbD::Constraint::type()
{
	return MbD::essential;
}

void MbD::Constraint::fillqsulam(FColDsptr col)
{
	col->at(iG) = lam;
}

void MbD::Constraint::setqsulam(FColDsptr col)
{
	lam = col->at(iG);
}
