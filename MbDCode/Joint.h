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

#include "Item.h"

namespace MbD {
	class Constraint;
	class EndFramec;
	using EndFrmsptr = std::shared_ptr<EndFramec>;

	class Joint : public Item
	{
		//frmI frmJ constraints friction 
	public:
		Joint();
		Joint(const char* str);

		void addConstraint(std::shared_ptr<Constraint> con);
		FColDsptr aFIeJtIe();
		FColDsptr aFIeJtO();
		FColDsptr aFX();
		FColDsptr aTIeJtIe();
		FColDsptr aTIeJtO();
		FColDsptr aTX();
		virtual void connectsItoJ(EndFrmsptr frmI, EndFrmsptr frmJ);
		void constraintsDo(const std::function <void(std::shared_ptr<Constraint>)>& f);
		void constraintsReport() override;
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
		void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints) override;
		void fillVelICError(FColDsptr col) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void initialize() override;
		void initializeGlobally() override;
		void initializeLocally() override;
		FColDsptr jointForceI();
		FColDsptr jointTorqueI();
		void postDynStep() override;
		void postInput() override;
		void postPosIC() override;
		void postPosICIteration() override;
		void preAccIC() override;
		void preDyn() override;
		void prePosIC() override;
		void prePosKine() override;
		void preVelIC() override;
		void reactivateRedundantConstraints() override;
		void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos) override;
		void setqsuddotlam(FColDsptr col) override;
		void setqsudotlam(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		std::shared_ptr<StateData> stateData() override;
		void useEquationNumbers() override;

		EndFrmsptr frmI;
		EndFrmsptr frmJ;
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> constraints;

	};
}

