#include "Sum.h"
#include "Constant.h"

using namespace MbD;

Symsptr MbD::Sum::timesSum(Symsptr sptr, Symsptr aSum)
{
	auto answer = std::make_shared<Sum>();
	auto sumTERMs = aSum->getTerms();
	for (const auto& term : *terms) {
		for (const auto& sumTERM : *sumTERMs) {
			Symsptr termTERM;
			if (sumTERM->isProduct()) {
				termTERM = term->timesProduct(term, sumTERM);
			}
			else {
				termTERM = term->timesFunction(term, sumTERM);
			}
			answer->terms->push_back(termTERM);
		}
	}
	return answer;
}

Symsptr MbD::Sum::timesProduct(Symsptr sptr, Symsptr product)
{
	return product->timesSum(product, sptr);
}

Symsptr MbD::Sum::timesFunction(Symsptr sptr, Symsptr function)
{
	return function->timesSum(function, sptr);
}

Symsptr MbD::Sum::expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
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

Symsptr MbD::Sum::simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
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

bool MbD::Sum::isSum()
{
	return true;
}

double Sum::getValue()
{
	double answer = 0.0;
	for (int i = 0; i < terms->size(); i++) answer += terms->at(i)->getValue();
	return answer;
}

std::ostream& MbD::Sum::printOn(std::ostream& s) const
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
