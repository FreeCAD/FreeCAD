/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "IndependentVariable.h"
#include "Constant.h"

using namespace MbD;

IndependentVariable::IndependentVariable()
{
}

Symsptr MbD::IndependentVariable::differentiateWRT(Symsptr var)
{
	if (this == var.get()) {
		return std::make_shared<Constant>(1.0);
	}
	else {
		return std::make_shared<Constant>(0.0);
	}
}
