/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include <functional>

#include "ItemIJ.h"

namespace MbD {
	class ConstraintSet : public ItemIJ
	{
		//
	public:
		ConstraintSet();
		ConstraintSet(const std::string& str);
		void constraintsDo(const std::function <void(std::shared_ptr<Constraint>)>& f);
		void initialize() override;
		void initializeGlobally() override;
		void initializeLocally() override;
		void addConstraint(std::shared_ptr<Constraint> con);
		void fillAccICIterError(FColDsptr col) override;
		void fillAccICIterJacob(SpMatDsptr mat) override;
		void fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints) override;
		void fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints) override;
		void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints) override;
		void fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineError(FColDsptr col) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillqsuddotlam(FColDsptr col) override;
		void fillqsulam(FColDsptr col) override;
		void fillqsudot(FColDsptr col) override;
		void fillqsudotWeights(DiagMatDsptr diagMat) override;
		void fillVelICError(FColDsptr col) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void postDynStep() override;
		void postInput() override;
		void postPosIC() override;
		void postPosICIteration() override;
		void preAccIC() override;
		void preDyn() override;
		void prePosIC() override;
		void prePosKine() override;
		void preVelIC() override;
		void setqsuddotlam(FColDsptr col) override;
		void setqsudotlam(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		void useEquationNumbers() override;


		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> constraints;

	};
}
