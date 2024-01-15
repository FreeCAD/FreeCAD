/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <memory>

#include "FullMatrix.h"

namespace MbD {
	class DifferenceOperator
	{
		//iStep order taylorMatrix operatorMatrix time timeNodes 
	public:
		virtual ~DifferenceOperator() {}
		void calcOperatorMatrix();
		virtual void initialize();
		virtual void initializeLocally();
		virtual void setiStep(size_t i);
		virtual void setorder(size_t o);
		virtual void formTaylorMatrix() = 0;
		virtual void instantiateTaylorMatrix();
		virtual void formTaylorRowwithTimeNodederivative(size_t i, size_t ii, size_t k);
		void settime(double t);

		size_t iStep = 0, order = 0;
		FMatDsptr taylorMatrix, operatorMatrix;
		double time = 0.0;
		std::shared_ptr<std::vector<double>> timeNodes;	//"Row of past times in order of increasing past."
		static FRowDsptr OneOverFactorials;
	};
}

