/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "AllowZRotationConstraintIqctJqc.h"
#include "EndFramec.h"
#include "MarkerFrame.h"
#include "EndFrameqc.h"
#include "CREATE.h"
#include "Symbolic.h"
#include "Constant.h"
 #include "EulerAngleszxz.h"

using namespace MbD;

MbD::AllowZRotationConstraintIqctJqc::AllowZRotationConstraintIqctJqc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj) :
	DirectionCosineConstraintIqctJqc(frmi, frmj, axisi, axisj)
{
	//Do nothing.
}

std::shared_ptr<AllowZRotationConstraintIqctJqc> MbD::AllowZRotationConstraintIqctJqc::With(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj)
{
	auto con = std::make_shared<AllowZRotationConstraintIqctJqc>(frmi, frmj, axisi, axisj);
	con->initialize();
	return con;
}

void MbD::AllowZRotationConstraintIqctJqc::postInput()
{
	auto eqctI = std::static_pointer_cast<EndFrameqct>(frmI);
	auto aAImJe = eqctI->getMarkerFrame()->aAOm->transposeTimesFullMatrix(frmJ->aAOe);
	auto aEulerAngleszxz = aAImJe->eulerAngleszxz();
	auto the1z = aEulerAngleszxz->at(1);
	auto the2x = aEulerAngleszxz->at(2);
	if (std::abs(the2x) < (M_PI / 2.0)) {
		eqctI->phiThePsiBlks->at(1) = std::make_shared<Constant>(the1z);
	}
	else {
		eqctI->phiThePsiBlks->at(1) = std::make_shared<Constant>(M_PI + the1z);
	}
	eqctI->postInput();
	DirectionCosineConstraintIqctJqc::postInput();
}

void MbD::AllowZRotationConstraintIqctJqc::postPosIC()
{
	//self becomeRedundantConstraint
	assert(false);
}
