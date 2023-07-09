#include "AngleJoint.h"
#include "CREATE.h"
#include "System.h"
#include "DirectionCosineConstraintIJ.h"

using namespace MbD;

MbD::AngleJoint::AngleJoint()
{
}

MbD::AngleJoint::AngleJoint(const char* str) : Joint(str)
{
}

void MbD::AngleJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 0));
		addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 1));
		addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 2));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
