/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <memory>
#include <vector>
#include <functional>

#include "ConstraintSet.h"

namespace MbD {
	class Constraint;
	class EndFramec;
	using EndFrmsptr = std::shared_ptr<EndFramec>;

	class Joint : public ConstraintSet
	{
		//frmI frmJ constraints friction 
	public:
		Joint();
		Joint(const std::string& str);
		virtual ~Joint() {}

		void initializeLocally() override;
		FColDsptr aFIeJtIe();
		FColDsptr aFIeJtO();
		FColDsptr aFX();
		FColDsptr aTIeJtIe();
		FColDsptr aTIeJtO();
		FColDsptr aTX();
		void constraintsReport() override;
		void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints) override;
		FColDsptr jointForceI();
		FColDsptr jointTorqueI();
		void reactivateRedundantConstraints() override;
		void removeRedundantConstraints(std::shared_ptr<std::vector<size_t>> redundantEqnNos) override;
		std::shared_ptr<StateData> stateData() override;

	};
}

