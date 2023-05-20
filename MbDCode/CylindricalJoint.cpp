#include "CylindricalJoint.h"
#include "System.h"
#include "DirectionCosineConstraintIJ.h"
#include "TranslationConstraintIJ.h"

using namespace MbD;

CylindricalJoint::CylindricalJoint() {
}

CylindricalJoint::CylindricalJoint(const char* str) : Joint(str) {
}

void MbD::CylindricalJoint::initializeGlobally()
{
	if (!constraints)
	{
		addConstraint(std::make_shared <TranslationConstraintIJ>(frmI, frmJ, 1));
		addConstraint(std::make_shared <TranslationConstraintIJ>(frmI, frmJ, 2));
		addConstraint(std::make_shared<DirectionCosineConstraintIJ>(frmI, frmJ, 3, 1));
		addConstraint(std::make_shared<DirectionCosineConstraintIJ>(frmI, frmJ, 3, 2));
		System::getInstance().hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
