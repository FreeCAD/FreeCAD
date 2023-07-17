#include "ZTranslation.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::ZTranslation::ZTranslation()
{
}

MbD::ZTranslation::ZTranslation(const char* str)
{
}

void MbD::ZTranslation::initializeGlobally()
{
	if (constraints->empty()) {
		initMotions();
		addConstraint(CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 0));
		this->root()->hasChanged = true;
	}
	else {
		PrescribedMotion::initializeGlobally();
	}
}

void MbD::ZTranslation::initMotions()
{
}
