#include "InPlaneJoint.h"
#include "CREATE.h"

MbD::InPlaneJoint::InPlaneJoint()
{
}

MbD::InPlaneJoint::InPlaneJoint(const char* str)
{
}

void MbD::InPlaneJoint::createInPlaneConstraint()
{
	auto tranCon = CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 0);
	tranCon->setConstant(offset);
	addConstraint(tranCon);
}
