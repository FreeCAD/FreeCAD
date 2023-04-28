#pragma once
#include <memory>
#include <vector>

#include "Constraint.h"
#include "FullRow.h"

namespace MbD {

	class EulerConstraint : public Constraint
	{
	public:
		EulerConstraint() : Constraint() {
		}
		//pGpE iqE 
		FullRow<double> pGpE = FullRow<double>(4);    //partial derivative of G wrt pE
		int iqE = -1;
	};
}

