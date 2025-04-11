/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "System.h"
#include "AllowZRotation.h"
#include "FullColumn.h"
#include "AllowZRotationConstraintIqctJqc.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"
#include "CREATE.h"
#include "RedundantConstraint.h"

using namespace MbD;

MbD::AllowZRotation::AllowZRotation()
{
	//Do nothing.
}

MbD::AllowZRotation::AllowZRotation(const std::string& str) : PrescribedMotion(str) 
{
	//Do nothing.
}

std::shared_ptr<AllowZRotation> MbD::AllowZRotation::With()
{
	auto allowZRotation = std::make_shared<AllowZRotation>();
	allowZRotation->initialize();
	return allowZRotation;
}

void MbD::AllowZRotation::initializeGlobally()
{
	if (constraints->empty()) {
		initMotions();
		auto dirCosCon = AllowZRotationConstraintIqctJqc::With(frmI, frmJ, 1, 0);
		addConstraint(dirCosCon);
		this->root()->hasChanged = true;
	}
	else {
		PrescribedMotion::initializeGlobally();
	}
}

void MbD::AllowZRotation::postPosIC()
{
	for (size_t i = 0; i < constraints->size(); i++)
	{
		auto& constraint = constraints->at(i);
		auto redunCon = CREATE<RedundantConstraint>::With();
		redunCon->constraint = constraint;
		constraints->at(i) = redunCon;
	}
}
