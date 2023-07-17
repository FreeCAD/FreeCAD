#include "ParallelAxesJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::ParallelAxesJoint::ParallelAxesJoint()
{
}

MbD::ParallelAxesJoint::ParallelAxesJoint(const char* str)
{
}

void MbD::ParallelAxesJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
