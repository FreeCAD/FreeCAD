#pragma once
#include <memory>
#include <vector>

#include "Constraint.h"
#include "FullRow.h"	//FRowDsptr is defined

namespace MbD {
	class EulerConstraint : public Constraint
	{
		//pGpE iqE 
	public:
		EulerConstraint();
		EulerConstraint(const char* str);
		void initialize();
		void calcPostDynCorrectorIteration() override;
		void useEquationNumbers() override;

		FRowDsptr pGpE;		//partial derivative of G wrt pE
		size_t iqE = -1;
	};
}

