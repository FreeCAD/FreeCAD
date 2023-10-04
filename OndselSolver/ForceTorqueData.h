/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "StateData.h"

namespace MbD {
	class ForceTorqueData : public StateData
	{
		//aFIO aTIO
	public:
		std::ostream& printOn(std::ostream& s) const override;

		FColDsptr aFIO, aTIO;
	};
}

