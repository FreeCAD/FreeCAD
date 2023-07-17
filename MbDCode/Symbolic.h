#pragma once

#include <iostream>
#include <unordered_set>
#include <memory>
#include <ostream>

#include "Math.h"

namespace MbD {

	class Symbolic : public Math
	{
	public:
		Symbolic();
		virtual void initialize();
		virtual std::shared_ptr<Symbolic> differentiateWRT(std::shared_ptr<Symbolic> sptr, std::shared_ptr<Symbolic> var);
		virtual std::shared_ptr<Symbolic> simplified(std::shared_ptr<Symbolic> sptr);
		virtual std::shared_ptr<Symbolic> expandUntil(std::shared_ptr<Symbolic> sptr, std::shared_ptr<std::unordered_set<std::shared_ptr<Symbolic>>> set);
		virtual std::shared_ptr<Symbolic> simplifyUntil(std::shared_ptr<Symbolic> sptr, std::shared_ptr<std::unordered_set<std::shared_ptr<Symbolic>>> set);
		virtual std::shared_ptr<Symbolic> timesSum(std::shared_ptr<Symbolic> sptr, std::shared_ptr<Symbolic> sum);
		virtual std::shared_ptr<Symbolic> timesProduct(std::shared_ptr<Symbolic> sptr, std::shared_ptr<Symbolic> product);
		virtual std::shared_ptr<Symbolic> timesFunction(std::shared_ptr<Symbolic> sptr, std::shared_ptr<Symbolic> function);
		virtual bool isSum();
		virtual bool isProduct();
		virtual bool isConstant();
		virtual std::ostream& printOn(std::ostream& s) const;

		virtual std::shared_ptr<std::vector<std::shared_ptr<Symbolic>>> getTerms();
		virtual double getValue();

		friend std::ostream& operator<<(std::ostream& s, const Symbolic& sym)
		{
			return sym.printOn(s);
		}
	};
	using Symsptr = std::shared_ptr<Symbolic>;
}

