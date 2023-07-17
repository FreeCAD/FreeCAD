#include "UniversalJoint.h"
#include "System.h"
#include "CREATE.h"

using namespace MbD;

MbD::UniversalJoint::UniversalJoint()
{
}

MbD::UniversalJoint::UniversalJoint(const char* str) : AtPointJoint(str)
{
}

void MbD::UniversalJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createAtPointConstraints();
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 2));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
