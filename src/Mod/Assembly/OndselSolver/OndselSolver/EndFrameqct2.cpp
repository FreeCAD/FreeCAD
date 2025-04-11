/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "EndFrameqct2.h"
#include "MarkerFrame.h"
#include "System.h"
#include "Symbolic.h"
#include "SymTime.h"
#include "EulerParameters.h"
#include "CREATE.h"
#include "EulerAngleszxz.h"
#include "EulerAngleszxzDot.h"
#include "EulerAngleszxzDDot.h"

using namespace MbD;

EndFrameqct2::EndFrameqct2() {
}

EndFrameqct2::EndFrameqct2(const std::string& str) : EndFrameqct(str) {
}

void EndFrameqct2::initpPhiThePsiptBlks()
{
	auto& mbdTime = this->root()->time;
	auto eulerAngles = std::static_pointer_cast<EulerAngles<Symsptr>>(phiThePsiBlks);
	//pPhiThePsiptBlks = differentiateWRT(*eulerAngles, mbdTime);
	pPhiThePsiptBlks = eulerAngles->differentiateWRT(mbdTime);
}

void EndFrameqct2::initppPhiThePsiptptBlks()
{
	auto& mbdTime = this->root()->time;
	auto eulerAnglesDot = std::static_pointer_cast<EulerAnglesDot<Symsptr>>(pPhiThePsiptBlks);
	ppPhiThePsiptptBlks = eulerAnglesDot->differentiateWRT(mbdTime);
}

void EndFrameqct2::evalAme()
{
	auto eulerAngles = std::static_pointer_cast<EulerAngles<Symsptr>>(phiThePsiBlks);
	eulerAngles->calc();
	aAme = eulerAngles->aA;
}

void EndFrameqct2::evalpAmept()
{
	auto eulerAnglesDot = std::static_pointer_cast<EulerAnglesDot<Symsptr>>(pPhiThePsiptBlks);
	eulerAnglesDot->calc();
	pAmept = eulerAnglesDot->aAdot;

}

void EndFrameqct2::evalppAmeptpt()
{
	auto eulerAnglesDDot = std::static_pointer_cast<EulerAnglesDDot<Symsptr>>(ppPhiThePsiptptBlks);
	eulerAnglesDDot->calc();
	ppAmeptpt = eulerAnglesDDot->aAddot;
}
