/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "EndFrameqct.h"
#include "Symbolic.h"
#include "EulerAngles.h"
#include "EulerAnglesDot.h"
#include "EulerAnglesDDot.h"

namespace MbD {
	class Time;

	class EndFrameqct2 : public EndFrameqct
	{
		//
	public:
		EndFrameqct2();
		EndFrameqct2(const std::string& str);
		void initpPhiThePsiptBlks() override;
		void initppPhiThePsiptptBlks() override;
		void evalAme() override;
		void evalpAmept() override;
		void evalppAmeptpt() override;

	};
}

