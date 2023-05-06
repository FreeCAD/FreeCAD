#include "Constraint.h"
//#include "Item.h"

using namespace MbD;

Constraint::Constraint()
{
	initialize();
}

Constraint::Constraint(const char* str) : Item(str)
{
	initialize();
}

void Constraint::initialize()
{
	iG = -1;
	aG = 0.0;
	lam = 0.0;
}

void Constraint::setOwner(Item* x)
{
	owner = x;
}

Item* Constraint::getOwner()
{
	return owner;
}
