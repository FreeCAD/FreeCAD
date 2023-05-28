#include "Constraint.h"
//#include "Item.h"

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
