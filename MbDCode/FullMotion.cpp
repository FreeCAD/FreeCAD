#include "FullMotion.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::FullMotion::FullMotion()
{
}

MbD::FullMotion::FullMotion(const char* str)
{
}

void MbD::FullMotion::initializeGlobally()
{
	if (constraints->empty()) {
		initMotions();
		addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 0));
		addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 1));
		addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 2));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 1, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 0));
		addConstraint(CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 1));
		this->root()->hasChanged = true;
	}
	else {
		PrescribedMotion::initializeGlobally();
	}
}

void MbD::FullMotion::initMotions()
{
}
