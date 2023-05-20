#pragma once

#include <unordered_set>
#include <memory>

namespace MbD {

	class Symbolic
	{
	public:
		Symbolic();
		void initialize();
		virtual std::shared_ptr<Symbolic> differentiateWRT(std::shared_ptr<Symbolic> var);
		virtual std::shared_ptr<Symbolic> simplified();
		virtual std::shared_ptr<Symbolic> expandUntil(std::shared_ptr<std::unordered_set<Symbolic>> set);
		virtual std::shared_ptr<Symbolic> simplifyUntil(std::shared_ptr<std::unordered_set<Symbolic>> set);
		virtual double getValue();
	};
}

