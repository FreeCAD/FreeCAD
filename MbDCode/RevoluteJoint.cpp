#include "RevoluteJoint.h"
#include "System.h"
#include "AtPointConstraintIJ.h"
#include "DirectionCosineConstraintIJ.h"
#include "CREATE.h"

using namespace MbD;

RevoluteJoint::RevoluteJoint() {

}

RevoluteJoint::RevoluteJoint(const char* str) : Joint(str) {

}

void RevoluteJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 0));
		addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 1));
		addConstraint(CREATE<AtPointConstraintIJ>::ConstraintWith(frmI, frmJ, 2));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
