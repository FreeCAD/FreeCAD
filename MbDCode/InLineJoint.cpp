#include "InLineJoint.h"
#include "CREATE.h"

MbD::InLineJoint::InLineJoint()
{
}

MbD::InLineJoint::InLineJoint(const char* str)
{
}

void MbD::InLineJoint::createInLineConstraints()
{
	addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 0));
	addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 1));
}
