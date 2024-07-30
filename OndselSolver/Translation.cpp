/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "Translation.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::Translation::Translation()
{
}

MbD::Translation::Translation(const std::string&)
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
