/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ZTranslation.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::ZTranslation::ZTranslation()
{
}

MbD::ZTranslation::ZTranslation(const std::string&)
{
}

void MbD::ZTranslation::initializeGlobally()
{
	if (constraints->empty()) {
		initMotions();
		auto tranCon = CREATE<TranslationConstraintIJ>::ConstraintWith(frmI, frmJ, 2);
		addConstraint(tranCon);
		this->root()->hasChanged = true;
	}
	else {
		PrescribedMotion::initializeGlobally();
	}
}
