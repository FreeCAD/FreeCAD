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
		void initialize();
		void postInput() override;
		void setOwner(Item* x);
		Item* getOwner();
		void prePosIC() override;
		virtual void fillEssenConstraints(std::shared_ptr<Constraint>sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints);
		virtual void fillDispConstraints(std::shared_ptr<Constraint>sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints);
		virtual void fillPerpenConstraints(std::shared_ptr<Constraint>sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints);
		virtual MbD::ConstraintType type();
		void fillqsulam(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		
		size_t iG;
		double aG;		//Constraint function
		double lam;		//Lambda is Lagrange Multiplier
		Item* owner;	//A Joint or PartFrame owns the constraint.  //Use raw pointer when pointing backwards.
	};
}

