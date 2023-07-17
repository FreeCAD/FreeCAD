#include "Orientation.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::Orientation::Orientation()
{
}

MbD::Orientation::Orientation(const char* str)
{
}

void MbD::Orientation::initializeGlobally()
{
	if (constraints->empty()) {
		initMotions();
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 1, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		PrescribedMotion::initializeGlobally();
	}
}

void MbD::Orientation::initMotions()
{
}
