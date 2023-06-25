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
		void initialize() override;
		void postInput() override;
		void setOwner(Item* x);
		Item* getOwner();
		void prePosIC() override;
		void prePosKine() override;
		virtual void fillEssenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints);
		virtual void fillDispConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints);
		virtual void fillPerpenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints);
		virtual void fillRedundantConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints);
		virtual void fillConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints);
		virtual MbD::ConstraintType type();
		void fillqsulam(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		void setqsudotlam(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos) override;
		void reactivateRedundantConstraints() override;
		virtual bool isRedundant();
		void outputStates() override;
		void preDyn() override;
		void fillPosKineError(FColDsptr col) override;
		void preAccIC() override;
		void fillAccICIterJacob(SpMatDsptr mat) override;
		void setqsuddotlam(FColDsptr qsudotlam) override;

		int iG = -1;
		double aG = 0.0;		//Constraint function
		double lam = 0.0;		//Lambda is Lagrange Multiplier
		double mu = 0, lamDeriv = 0;
		Item* owner;	//A Joint or PartFrame owns the constraint.  //Use raw pointer when pointing backwards.
	};
}

