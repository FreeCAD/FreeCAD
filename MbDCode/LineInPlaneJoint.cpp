#include "LineInPlaneJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::LineInPlaneJoint::LineInPlaneJoint()
{
}

MbD::LineInPlaneJoint::LineInPlaneJoint(const char* str)
{
}

void MbD::LineInPlaneJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		this->createInPlaneConstraint();
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 2));
		this->root()->hasChanged = true;
	}
	else {
		InPlaneJoint::initializeGlobally();
	}
}
