/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <sstream>
#include <string>

#include "Sum.h"
#include "Constant.h"
#include <algorithm>

using namespace MbD;

Symsptr MbD::Sum::parseExpression(std::string& expression)
{
	std::istringstream iss(expression);
	auto sum = std::make_shared<Sum>();
	sum->parse(iss);
	return sum->simplified(sum);
}

void MbD::Sum::parse(std::istringstream& iss)
{
	iss >> std::ws;
	char c = iss.peek();
	if (c == '+') {
		parsePlusTerm(iss);
	}
	else 	if (c == '-') {
		parseMinusTerm(iss);
	}
	else {
		parseTerm(iss);
	}
}

void MbD::Sum::parseTerm(std::istringstream& iss)
{
}

void MbD::Sum::parsePlusTerm(std::istringstream& iss)
{
	iss.get();

}

void MbD::Sum::parseMinusTerm(std::istringstream& iss)
{
}

Symsptr Sum::expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	auto itr = std::find_if(set->begin(), set->end(), [sptr](Symsptr sym) {return sptr.get() == sym.get(); });
	if (itr != set->end()) {
		auto answer = std::make_shared<Sum>();
		answer->terms = terms;
		return answer;
	}
	auto newTerms = std::make_shared<std::vector<Symsptr>>();
	for (const auto& term : *terms) {
		auto newTerm = term->expandUntil(term, set);
		if (newTerm->isSum()) {
			newTerms->insert(newTerms->end(), newTerm->getTerms()->begin(), newTerm->getTerms()->end());
		}
		else {
			newTerms->push_back(newTerm);
		}
	}
	auto newSize = newTerms->size();
	if (newSize == 0) {
		return std::make_shared<Constant>(0.0);
	}
	else if (newSize == 1) {
		return newTerms->at(0);
	}
	else {
		auto answer = std::make_shared<Sum>();
		answer->terms = newTerms;
		return answer;
	}
}

Symsptr Sum::simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	auto itr = std::find_if(set->begin(), set->end(), [this](Symsptr sym) {return this == (sym.get()); });
	if (itr != set->end()) {
		auto answer = std::make_shared<Sum>();
		answer->terms = terms;
		return answer;
	}
	auto newTerms = std::make_shared<std::vector<Symsptr>>();
	double constant = 0.0;
	for (const auto& term : *terms) {
		auto newTerm = term->simplifyUntil(term, set);
		if (newTerm->isConstant()) {
			constant += term->getValue();
		}
		else {
			newTerms->push_back(newTerm);
		}
	}
	if (constant != 0.0) {
		newTerms->insert(newTerms->begin(), std::make_shared<Constant>(constant));
	}
	auto newSize = newTerms->size();
	if (newSize == 0) {
		return std::make_shared<Constant>(0.0);
	}
	else if (newSize == 1) {
		return newTerms->at(0);
	}
	else {
		auto answer = std::make_shared<Sum>();
		answer->terms = newTerms;
		return answer;
	}
}

bool Sum::isSum()
{
	return true;
}

double Sum::getValue()
{
	double answer = 0.0;
	for (int i = 0; i < terms->size(); i++) answer += terms->at(i)->getValue();
	return answer;
}

Symsptr MbD::Sum::clonesptr()
{
	return std::make_shared<Sum>(*this);
}

Symsptr MbD::Sum::differentiateWRT(Symsptr var)
{
	auto derivatives = std::make_shared<std::vector<Symsptr>>();
	std::transform(terms->begin(), terms->end(), 
		std::back_inserter(*derivatives),
		[var](Symsptr term) { return term->differentiateWRT(var); }
	);
	auto answer = std::make_shared<Sum>();
	answer->terms = derivatives;
	return answer;
}

std::ostream& Sum::printOn(std::ostream& s) const
{
	s << "(";
	s << *(this->terms->at(0));
	for (int i = 1; i < this->terms->size(); i++)
	{
		s << " + " << *(this->terms->at(i));
	}
	s << ")";
	return s;
}
