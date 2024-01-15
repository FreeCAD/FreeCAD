/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <algorithm>
#include <iterator>

#include "Product.h"
#include "Sum.h"
#include "Constant.h"

using namespace MbD;

Symsptr MbD::Product::differentiateWRT(Symsptr var)
{
	//"Apply chain rule of differentiation."
	//	"(xyz)' := x'yz + xy'z + xyz'."

	auto derivatives = std::make_shared<std::vector<Symsptr>>();
	std::transform(terms->begin(),
		terms->end(),
		std::back_inserter(*derivatives),
		[var](Symsptr term) { 
			return term->differentiateWRT(var);
		}
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

Symsptr MbD::Product::integrateWRT(Symsptr var)
{
	//ToDo: Integration by parts
	auto newTerms = std::make_shared<std::vector<Symsptr>>();
	double factor = 1.0;
	for (const auto& term : *terms) {
		if (term->isConstant()) {
			factor *= term->getValue();
		}
		else {
			newTerms->push_back(term);
		}
	}
	if (factor == 0.0) {
		return sptrConstant(0.0);
	}
	if (factor == 1.0) {
		assert(newTerms->size() == 1);
		return newTerms->front()->integrateWRT(var);
	}
	assert(newTerms->size() == 1);
	auto soleIntegral = newTerms->front()->integrateWRT(var);
	auto answer = std::make_shared<Product>();
	answer->terms->push_back(sptrConstant(factor));
	answer->terms->push_back(soleIntegral);
	return answer;
}

Symsptr Product::expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
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
	//sumOfProductsOfSums = (a + b + ...)*(c + d + ...)
	auto sumOfProductsOfSums = std::make_shared<Sum>(sptrConstant(1));
	for (const auto& term : *sumTerms) {
		sumOfProductsOfSums = std::static_pointer_cast<Sum>(Symbolic::times(sumOfProductsOfSums, term));
	}
	return Symbolic::times(factor, sumOfProductsOfSums);
}

Symsptr Product::simplifyUntil(Symsptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
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
		return sptrConstant(0.0);
	}
	if (factor != 1.0) {
		newTerms->insert(newTerms->begin(), sptrConstant(factor));
	}
	auto newSize = newTerms->size();
	if (newSize == 0) {
		return sptrConstant(1.0);
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

std::ostream& Product::printOn(std::ostream& s) const
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

bool Product::isProduct()
{
	return true;
}

double Product::getValue()
{
	double answer = 1.0;
	for (size_t i = 0; i < terms->size(); i++) answer *= terms->at(i)->getValue();
	return answer;
}

Symsptr MbD::Product::clonesptr()
{
	return std::make_shared<Product>(*this);
}
