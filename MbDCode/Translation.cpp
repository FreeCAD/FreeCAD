#include "Translation.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::Translation::Translation()
{
}

MbD::Translation::Translation(const char* str)
{
}

void MbD::Translation::initializeGlobally()
{
	if (constraints->empty()) {
		initMotions();
		addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 0));
		addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 1));
		addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 2));
		this->root()->hasChanged = true;
	}
	else {
		PrescribedMotion::initializeGlobally();
	}
}

void MbD::Translation::initMotions()
{
}
