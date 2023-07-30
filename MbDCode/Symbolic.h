#pragma once

#include <iostream>
#include <unordered_set>
#include <memory>
#include <ostream>

#include "Math.h"
#include "System.h"
#include "Units.h"

namespace MbD {

	class Symbolic : public Math
	{
	public:
		Symbolic();
		static std::shared_ptr<Symbolic> times(std::shared_ptr<Symbolic> arg, std::shared_ptr<Symbolic> arg1);
		static std::shared_ptr<Symbolic> sum(std::shared_ptr<Symbolic> arg, std::shared_ptr<Symbolic> arg1);
		static std::shared_ptr<Symbolic> raisedTo(std::shared_ptr<Symbolic> x, std::shared_ptr<Symbolic> y);

		virtual void initialize();
		virtual std::shared_ptr<Symbolic> differentiateWRT(std::shared_ptr<Symbolic> var);
		virtual std::shared_ptr<Symbolic> simplified();
		virtual std::shared_ptr<Symbolic> simplified(std::shared_ptr<Symbolic> sptr);
		virtual std::shared_ptr<Symbolic> expandUntil(std::shared_ptr<std::unordered_set<std::shared_ptr<Symbolic>>> set);
		virtual std::shared_ptr<Symbolic> expandUntil(std::shared_ptr<Symbolic> sptr, std::shared_ptr<std::unordered_set<std::shared_ptr<Symbolic>>> set);
		virtual std::shared_ptr<Symbolic> simplifyUntil(std::shared_ptr<Symbolic> sptr, std::shared_ptr<std::unordered_set<std::shared_ptr<Symbolic>>> set);
		virtual bool isZero();
		virtual bool isOne();
		virtual bool isSum();
		virtual bool isProduct();
		virtual bool isConstant();
		virtual std::shared_ptr<std::vector<std::shared_ptr<Symbolic>>> getTerms();
		virtual void addTerm(std::shared_ptr<Symbolic> trm);
		virtual double getValue();
		virtual void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits);
		virtual std::shared_ptr<Symbolic> clonesptr();
		virtual void arguments(std::shared_ptr<Symbolic> args);
		virtual std::ostream& printOn(std::ostream& s) const;
		friend std::ostream& operator<<(std::ostream& s, const Symbolic& sym)
		{
			return sym.printOn(s);
		}
	};
	using Symsptr = std::shared_ptr<Symbolic>;
}

