#pragma once
#include <memory>

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

		
		int iG;
		double aG;		//Constraint function
		double lam;		//Lambda is Lagrange Multiplier
		Item* owner;	//A Joint or PartFrame owns the constraint
	};
}

