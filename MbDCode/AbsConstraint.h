#pragma once
#include "Constraint.h"
namespace MbD {
	class AbsConstraint : public Constraint
	{
		//axis iqXminusOnePlusAxis 
	public:
		//AbsConstraint();
		//AbsConstraint(const char* str);
		AbsConstraint(int axis);

		void calcPostDynCorrectorIteration() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void useEquationNumbers() override;

		int axis = -1;
		int iqXminusOnePlusAxis = -1;
	};
}

