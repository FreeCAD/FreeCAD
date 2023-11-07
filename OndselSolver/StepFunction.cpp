/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <algorithm>

#include "StepFunction.h"
#include "Constant.h"

using namespace MbD;

MbD::StepFunction::StepFunction(Symsptr var, std::shared_ptr<std::vector<double>> consts, std::shared_ptr<std::vector<double>> trans)
{
	xx = var;
	std::transform(consts->begin(), consts->end(), functions->begin(),
		[&](auto& constant) { return sptrConstant(constant); }
	);
	std::transform(trans->begin(), trans->end(), transitions->begin(),
		[&](auto& constant) { return sptrConstant(constant); }
	);
}
