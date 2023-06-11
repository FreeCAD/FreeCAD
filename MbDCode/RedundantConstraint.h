#pragma once

#include "Constraint.h"

namespace MbD {
	class RedundantConstraint : public Constraint
	{
		//
	public:
		void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos) override;
		bool isRedundant() override;
		std::string classname() override;
		MbD::ConstraintType type() override;
		void fillqsulam(FColDsptr col) override;
		void postInput() override;
		void prePosIC() override;
		void fillEssenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints);
		void fillDispConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints);
		void fillPerpenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints);
		void fillRedundantConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints);
		void setqsulam(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;

		std::shared_ptr<Constraint> constraint;
	};
}

