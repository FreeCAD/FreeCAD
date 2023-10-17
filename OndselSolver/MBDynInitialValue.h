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
		void readDerivativesTolerance(std::vector<std::string>& lines);
		void readDerivativesMaxIterations(std::vector<std::string>& lines);
		void readDerivativesCoefficient(std::vector<std::string>& lines);
		void createASMT() override;

		double initialTime = 0.0, finalTime = 5.0, timeStep = 1.0e-2, tolerance = 1.0e-6;
		int maxIterations = 10;
		double derivativesTolerance = 1.0e-4;
		int derivativesMaxIterations = 100;
		std::string derivativesCoefficient = "auto";
	};
}
