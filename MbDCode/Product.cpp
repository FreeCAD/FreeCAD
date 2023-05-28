#include <algorithm>
#include <iterator>

#include "Product.h"
#include "Sum.h"
#include "Constant.h"

using namespace MbD;

Symsptr MbD::Product::differentiateWRT(Symsptr sptr, Symsptr var)
{
	//"Apply chain rule of differentiation."
	//	"(xyz)' := x'yz + xy'z + xyz'."

	auto derivatives = std::make_shared<std::vector<Symsptr>>();
	std::transform(terms->begin(),
		terms->end(),
		std::back_inserter(*derivatives),
		[var](Symsptr term) { return term->differentiateWRT(term, var); }
	);
	auto derivativeTerms = std::make_shared<std::vector<Symsptr>>();
	for (size_t i = 0; i < terms->size(); i++)
	{
		auto& derivative = derivatives->at(i);
		auto newTermFunctions = std::make_shared<std::vector<Symsptr>>(*terms);
		newTermFunctions->at(i) = derivative;
		auto newTerm = std::make_shared<Product>();
		newTerm->terms = newTermFunctions;
		derivativeTerms->push_back(newTerm);
	}
	auto answer = std::make_shared<Sum>();
	answer->terms = derivativeTerms;
	return answer;
}

Symsptr MbD::Product::expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	auto itr = std::find_if(set->begin(), set->end(), [sptr](Symsptr sym) {return sptr.get() == sym.get(); });
	if (itr != set->end()) {
		auto answer = std::make_shared<Sum>();
		answer->terms = terms;
		return answer;
	}
	auto sumTerms = std::make_shared<std::vector<Symsptr>>();
	auto productTerms = std::make_shared<std::vector<Symsptr>>();
	for (const auto& term : *terms) {
		auto newTerm = term->expandUntil(term, set);
		if (newTerm->isSum()) {
			sumTerms->push_back(newTerm);
		}
		else if (newTerm->isProduct()) {
			productTerms->insert(productTerms->end(), newTerm->getTerms()->begin(), newTerm->getTerms()->end());
		}
		else {
			productTerms->push_back(newTerm);
		}
	}
	auto factor = std::make_shared<Product>();
	factor->terms = productTerms;
	auto sumOfProductsOfSums = std::make_shared<Sum>(std::make_shared<Constant>(1));
	for (const auto& term : *sumTerms) {
		sumOfProductsOfSums = std::static_pointer_cast<Sum>(sumOfProductsOfSums->timesSum(sumOfProductsOfSums, term));
	}
	return factor->timesSum(factor, sumOfProductsOfSums);
}

Symsptr MbD::Product::simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	auto itr = std::find_if(set->begin(), set->end(), [this](Symsptr sym) {return this == (sym.get()); });
	if (itr != set->end()) {
		auto answer = std::make_shared<Sum>();
		answer->terms = terms;
		return answer;
	}
	auto newTerms = std::make_shared<std::vector<Symsptr>>();
	double factor = 1.0;
	for (const auto& term : *terms) {
		auto newTerm = term->simplifyUntil(term, set);
		if (newTerm->isConstant()) {
			factor *= term->getValue();
		}
		else {
			newTerms->push_back(newTerm);
		}
	}
	if (factor == 0.0) {
		return std::make_shared<Constant>(0.0);
	}
	if (factor != 1.0) {
		newTerms->insert(newTerms->begin(), std::make_shared<Constant>(factor));
	}
	auto newSize = newTerms->size();
	if (newSize == 0) {
		return std::make_shared<Constant>(1.0);
	}
	else if (newSize == 1) {
		return newTerms->at(0);
	}
	else {
		auto answer = std::make_shared<Product>();
		answer->terms = newTerms;
		return answer;
	}
}

Symsptr MbD::Product::timesSum(Symsptr sptr, Symsptr aSum)
{
	auto answer = std::make_shared<Sum>();
	auto sumTERMs = aSum->getTerms();
	for (const auto& sumTERM : *sumTERMs) {
		Symsptr termTERM;
		if (sumTERM->isProduct()) {
			termTERM = sptr->timesProduct(sptr, sumTERM);
		}
		else {
			termTERM = sptr->timesFunction(sptr, sumTERM);
		}
		answer->terms->push_back(termTERM);
	}
	return answer;
}

Symsptr MbD::Product::timesProduct(Symsptr sptr, Symsptr product)
{
	auto answer = std::make_shared<Product>(sptr->getTerms());
	auto& answerTerms = answer->terms;
	auto productTerms = product->getTerms();
	answerTerms->insert(answerTerms->end(), productTerms->begin(), productTerms->end());
	return answer;
}

Symsptr MbD::Product::timesFunction(Symsptr sptr, Symsptr function)
{
	auto answer = std::make_shared<Product>(sptr->getTerms());
	answer->terms->push_back(function);
	return answer;
}

std::ostream& MbD::Product::printOn(std::ostream& s) const
{
	s << "(";
	s << *(this->terms->at(0));
	for (size_t i = 1; i < this->terms->size(); i++)
	{
		s << "*" << *(this->terms->at(i));
	}
	s << ")";
	return s;
}

bool MbD::Product::isProduct()
{
	return true;
}

double Product::getValue()
{
	double answer = 1.0;
	for (size_t i = 0; i < terms->size(); i++) answer *= terms->at(i)->getValue();
	return answer;
}
