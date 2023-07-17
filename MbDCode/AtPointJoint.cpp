#include "AtPointJoint.h"
#include "System.h"
#include "CREATE.h"

using namespace MbD;

MbD::AtPointJoint::AtPointJoint()
{
}

MbD::AtPointJoint::AtPointJoint(const char* str) : Joint(str)
{
}

void MbD::AtPointJoint::createAtPointConstraints()
{
	addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 0));
	addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 1));
	addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 2));
}
