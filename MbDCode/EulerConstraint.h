#pragma once
#include <memory>
#include <vector>

#include "Constraint.h"
#include "FullRow.h"

namespace MbD {
	using FullRowDPtr = std::shared_ptr<FullRow<double>>;

	class EulerConstraint : public Constraint
	{
	public:
		EulerConstraint() : Constraint() {
			pGpE = std::make_shared<FullRow<double>>(4);
		}
		//pGpE iqE 
		FullRowDPtr pGpE;    //partial derivative of G wrt pE
		int iqE;
	};
}

