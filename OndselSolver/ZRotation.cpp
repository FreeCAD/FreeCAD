/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "System.h"
#include "ZRotation.h"
#include "FullColumn.h"
#include "DirectionCosineConstraintIJ.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"
#include "CREATE.h"

using namespace MbD;

ZRotation::ZRotation() {

}

ZRotation::ZRotation(const char* str) : PrescribedMotion(str) {

}

void ZRotation::initializeGlobally()
{
	if (constraints->empty()) {
		initMotions();
		auto dirCosCon = CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 1, 0);
		addConstraint(dirCosCon);
		this->root()->hasChanged = true;
	}
	else {
		PrescribedMotion::initializeGlobally();
	}
}
