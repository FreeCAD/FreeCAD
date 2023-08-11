/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTItem.h"

namespace MbD {
	class ASMTSimulationParameters : public ASMTItem
	{
		//
	public:
		void parseASMT(std::vector<std::string>& lines) override;

		double tstart = 0.0, tend = 1.0, hmin = 1.0e-9, hmax = 1.0e9, hout = 0.1, errorTol = 1.0e-6;
		double errorTolPosKine = 1.0e-6, errorTolAccKine = 1.0e-6, corAbsTol = 1.0e-6, corRelTol = 1.0e-6;
		double intAbsTol = 1.0e-6, intRelTol = 1.0e-6, translationLimit = 1.0e9, rotationLimit = 1.0e9;
		int iterMaxPosKine = 25, iterMaxAccKine = 25, iterMaxDyn = 4, orderMax = 5;
	};
}

