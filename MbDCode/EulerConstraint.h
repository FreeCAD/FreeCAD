#pragma once
#include <memory>
#include <vector>

#include "Constraint.h"
#include "FullRow.h"

namespace MbD {
	class EulerConstraint : public Constraint
	{
		//pGpE iqE 
	public:
		EulerConstraint();
		EulerConstraint(const char* str);
		void initialize();

		FullRowDptr pGpE = std::make_shared<FullRow<double>>(4);    //partial derivative of G wrt pE
		int iqE = -1;
	};
}

