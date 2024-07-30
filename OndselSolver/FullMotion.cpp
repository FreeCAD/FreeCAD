/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "FullMotion.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::FullMotion::FullMotion()
{
}

MbD::FullMotion::FullMotion(const std::string&)
{
}

void MbD::FullMotion::connectsItoJ(EndFrmsptr frmi, EndFrmsptr frmj)
{
	Joint::connectsItoJ(frmi, frmj);
	std::static_pointer_cast<EndFrameqc>(frmI)->initEndFrameqct2();
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
	auto efrmI = std::static_pointer_cast<EndFrameqct>(frmI);
	efrmI->rmemBlks = frIJI;
	efrmI->phiThePsiBlks = fangIJJ;
}
