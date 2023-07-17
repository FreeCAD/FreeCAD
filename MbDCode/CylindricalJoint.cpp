#include "CylindricalJoint.h"
#include "System.h"
#include "DirectionCosineConstraintIJ.h"
#include "TranslationConstraintIJ.h"
#include "CREATE.h"

using namespace MbD;

CylindricalJoint::CylindricalJoint() 
{
}

CylindricalJoint::CylindricalJoint(const char* str) : InLineJoint(str) 
{
}

void CylindricalJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createInLineConstraints();
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		InLineJoint::initializeGlobally();
	}
}
