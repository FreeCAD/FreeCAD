#include "PerpendicularJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::PerpendicularJoint::PerpendicularJoint()
{
}

MbD::PerpendicularJoint::PerpendicularJoint(const char* str)
{
}

void MbD::PerpendicularJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 2));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
