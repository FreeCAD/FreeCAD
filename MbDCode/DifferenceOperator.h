#pragma once

#include <memory>

#include "FullMatrix.h"

namespace MbD {
	class DifferenceOperator
	{
		//iStep order taylorMatrix operatorMatrix time timeNodes 
	public:
		void calcOperatorMatrix();
		void initialize();
		virtual void setiStep(int i);
		virtual void setorder(int o);
		virtual void formTaylorMatrix() = 0;
		virtual void instanciateTaylorMatrix();
		virtual void formTaylorRowwithTimeNodederivative(int i, int ii, int k);

		int iStep = 0, order = 0;
		std::shared_ptr<FullMatrix<double>> taylorMatrix, operatorMatrix;
		double time = 0;
		std::shared_ptr<std::vector<double>> timeNodes;
	};
}

