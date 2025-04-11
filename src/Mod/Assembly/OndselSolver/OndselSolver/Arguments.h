/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "FunctionWithManyArgs.h"

namespace MbD {
	class Arguments : public FunctionWithManyArgs
	{
	public:
		void arguments(Symsptr args) override;
	};
}
