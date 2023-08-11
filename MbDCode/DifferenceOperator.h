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
		void calcOperatorMatrix();
		virtual void initialize();
		virtual void setiStep(int i);
		virtual void setorder(int o);
		virtual void formTaylorMatrix() = 0;
		virtual void instantiateTaylorMatrix();
		virtual void formTaylorRowwithTimeNodederivative(int i, int ii, int k);
		void settime(double t);

		int iStep = 0, order = 0;
		FMatDsptr taylorMatrix, operatorMatrix;
		double time = 0;
		std::shared_ptr<std::vector<double>> timeNodes;
		static FRowDsptr OneOverFactorials;
	};
}

