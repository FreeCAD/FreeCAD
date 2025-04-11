/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include <cstdint>
#include <memory>

#include "enum.h"
#include "Item.h"

namespace MbD {
	class Constraint : public Item
	{
		//iG aG lam mu lamDeriv owner 
	public:
		Constraint();
		Constraint(const std::string& str);

		void initialize() override;
		virtual void addToJointForceI(FColDsptr col);
		virtual void addToJointTorqueI(FColDsptr col);
		void fillAccICIterJacob(SpMatDsptr mat) override;
        void fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints) override;
		virtual void fillConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints);
        void fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints) override;
		virtual void fillDispConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints);
        void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints) override;
		virtual void fillEssenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints);
        void fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints) override;
		virtual void fillPerpenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints);
		void fillPosICError(FColDsptr col) override;
		void fillPosKineError(FColDsptr col) override;
		void fillqsuddotlam(FColDsptr col) override;
		void fillqsulam(FColDsptr col) override;
        void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints) override;
		virtual void fillRedundantConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints);
		virtual bool isRedundant();
		void postInput() override;
		void preAccIC() override;
		void preDyn() override;
		void prePosIC() override;
		void prePosKine() override;
		void reactivateRedundantConstraints() override;
		void removeRedundantConstraints(std::shared_ptr<std::vector<size_t>> redundantEqnNos) override;
		void setConstant(double value);
		void setqsudotlam(FColDsptr col) override;
		void setqsuddotlam(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		virtual ConstraintType type();
		
		size_t iG = SIZE_MAX;
		double aG = 0.0;		//Constraint function
		double aConstant = 0.0;
		double lam = 0.0;		//Lambda is Lagrange Multiplier
		double mu = 0.0, lamDeriv = 0.0;
	};
}

