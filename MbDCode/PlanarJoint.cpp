#include "PlanarJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::PlanarJoint::PlanarJoint()
{
}

MbD::PlanarJoint::PlanarJoint(const char* str)
{
}

void MbD::PlanarJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		this->createInPlaneConstraint();
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		InPlaneJoint::initializeGlobally();
	}
}
