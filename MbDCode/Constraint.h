#pragma once

#include <memory>

#include "enum.h"
#include "Item.h"

namespace MbD {
	class Constraint : public Item
	{
		//iG aG lam mu lamDeriv owner 
	public:
		Constraint();
		Constraint(const char* str);

		virtual void addToJointForceI(FColDsptr col);
		virtual void addToJointTorqueI(FColDsptr col);
		void fillAccICIterJacob(SpMatDsptr mat) override;
		virtual void fillConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints);
		virtual void fillDispConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints);
		virtual void fillEssenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints);
		virtual void fillPerpenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints);
		void fillPosICError(FColDsptr col) override;
		void fillPosKineError(FColDsptr col) override;
		void fillqsuddotlam(FColDsptr col) override;
		void fillqsulam(FColDsptr col) override;
		virtual void fillRedundantConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints);
		void initialize() override;
		virtual bool isRedundant();
		void postInput() override;
		void preAccIC() override;
		void preDyn() override;
		void prePosIC() override;
		void prePosKine() override;
		void reactivateRedundantConstraints() override;
		void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos) override;
		virtual void setConstant(double value);
		void setqsudotlam(FColDsptr col) override;
		void setqsuddotlam(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		virtual ConstraintType type();
		
		int iG = -1;
		double aG = 0.0;		//Constraint function
		double lam = 0.0;		//Lambda is Lagrange Multiplier
		double mu = 0, lamDeriv = 0;
	};
}

