#include "RevCylJoint.h"
#include "CREATE.h"
#include "DistancexyConstraintIJ.h"
#include "System.h"

using namespace MbD;

MbD::RevCylJoint::RevCylJoint()
{
}

MbD::RevCylJoint::RevCylJoint(const char* str) : CompoundJoint(str)
{
}

void MbD::RevCylJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto distxyIJ = CREATE<DistancexyConstraintIJ>::With(frmI, frmJ);
		distxyIJ->setConstant(distanceIJ);
		addConstraint(distxyIJ);
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		CompoundJoint::initializeGlobally();
	}
}
