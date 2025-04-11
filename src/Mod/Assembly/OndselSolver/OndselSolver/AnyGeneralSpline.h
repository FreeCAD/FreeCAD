/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "FunctionFromData.h"

namespace MbD {
	class AnyGeneralSpline : public FunctionFromData
	{
		//derivs degree index delta 
	public:
		AnyGeneralSpline() = default;
		AnyGeneralSpline(Symsptr arg);
	};
}
