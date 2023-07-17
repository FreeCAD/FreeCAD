#include "FixedJoint.h"
#include "System.h"
#include "CREATE.h"

using namespace MbD;

MbD::FixedJoint::FixedJoint()
{
}

MbD::FixedJoint::FixedJoint(const char* str) : AtPointJoint(str)
{
}

void MbD::FixedJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createAtPointConstraints();
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 1, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
