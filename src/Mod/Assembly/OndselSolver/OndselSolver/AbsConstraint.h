/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>

#include "Constraint.h"
namespace MbD {
	class AbsConstraint : public Constraint
	{
		//axis iqXminusOnePlusAxis 
	public:
		//AbsConstraint();
		//AbsConstraint(const std::string& str);
		AbsConstraint(size_t axis);

		void calcPostDynCorrectorIteration() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void useEquationNumbers() override;

		size_t axis = SIZE_MAX;
		size_t iqXminusOnePlusAxis = SIZE_MAX;
	};
}

