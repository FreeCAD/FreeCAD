/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include "MBDynBlock.h"

namespace MbD {
	class MBDynInitialValue : public MBDynBlock
	{
	public:
		void initialize() override;
		void parseMBDyn(std::vector<std::string>& lines) override;
		void readInitialTime(std::vector<std::string>& lines);
		void readFinalTime(std::vector<std::string>& lines);
		void readTimeStep(std::vector<std::string>& lines);
		void readMaxIterations(std::vector<std::string>& lines);
		void readTolerance(std::vector<std::string>& lines);

		double initialTime, finalTime, timeStep, tolerance;
		int maxIterations;
	};
}
