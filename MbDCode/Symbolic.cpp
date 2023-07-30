#include <memory>
#include <unordered_set>
#include <assert.h>

#include "Symbolic.h"
#include "Constant.h"
#include "Product.h"
#include "Sum.h"
#include "Power.h"

using namespace MbD;

Symbolic::Symbolic()
{
}

Symsptr MbD::Symbolic::times(Symsptr arg, Symsptr arg1)
{
	if (arg->isProduct()) {
		if (arg1->isProduct()) {
			auto newTerms = arg->getTerms();
			auto arg1Terms = arg1->getTerms();
			newTerms->insert(newTerms->end(), arg1Terms->begin(), arg1Terms->end());
			return std::make_shared<Product>(newTerms);
		}
		else {
			auto newTerms = arg->getTerms();
			newTerms->insert(newTerms->end(), arg1);
			return std::make_shared<Product>(newTerms);
		}
	}
	else {
		if (arg1->isProduct()) {
			auto newTerms = arg1->getTerms();
			newTerms->insert(newTerms->begin(), arg);
			return std::make_shared<Product>(newTerms);
		}
		else {
			return std::make_shared<Product>(arg, arg1);
		}
	}
}

Symsptr MbD::Symbolic::sum(Symsptr arg, Symsptr arg1)
{
	if (arg->isSum()) {
		if (arg1->isSum()) {
			auto newTerms = arg->getTerms();
			auto arg1Terms = arg1->getTerms();
			newTerms->insert(newTerms->end(), arg1Terms->begin(), arg1Terms->end());
			return std::make_shared<Sum>(newTerms);
		}
		else {
			auto newTerms = arg->getTerms();
			newTerms->insert(newTerms->end(), arg1);
			return std::make_shared<Sum>(newTerms);
		}
	}
	else {
		if (arg1->isSum()) {
			auto newTerms = arg1->getTerms();
			newTerms->insert(newTerms->begin(), arg);
			return std::make_shared<Sum>(newTerms);
		}
		else {
			return std::make_shared<Sum>(arg, arg1);
		}
	}
}

void Symbolic::initialize()
{
}

Symsptr MbD::Symbolic::differentiateWRT(Symsptr var)
{
	assert(false);
	return Symsptr();
}

Symsptr MbD::Symbolic::simplified()
{
	//std::cout << "sptr " << *sptr << std::endl;
	auto set = std::make_shared<std::unordered_set<Symsptr>>();
	auto expanded = this->expandUntil(set);
	//std::cout << "expanded " << *expanded << std::endl;
	auto set1 = std::make_shared<std::unordered_set<Symsptr>>();
	auto simplified = expanded->simplifyUntil(expanded, set1);
	//std::cout << "simplified " << *simplified << std::endl;
	return simplified;
}

Symsptr Symbolic::simplified(Symsptr sptr)
{
	//std::cout << "sptr " << *sptr << std::endl;
	auto set = std::make_shared<std::unordered_set<Symsptr>>();
	auto expanded = sptr->expandUntil(sptr, set);
	//std::cout << "expanded " << *expanded << std::endl;
	auto set1 = std::make_shared<std::unordered_set<Symsptr>>();
	auto simplified = expanded->simplifyUntil(expanded, set1);
	//std::cout << "simplified " << *simplified << std::endl;
	return simplified;
}

Symsptr MbD::Symbolic::expandUntil(std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	assert(false);
	return clonesptr();
}

Symsptr Symbolic::expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	return sptr;
}

Symsptr Symbolic::simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	return sptr;
}

bool MbD::Symbolic::isZero()
{
	return false;
}

bool MbD::Symbolic::isOne()
{
	return false;
}

bool Symbolic::isSum()
{
	return false;
}

bool Symbolic::isProduct()
{
	return false;
}

bool Symbolic::isConstant()
{
	return false;
}

std::ostream& Symbolic::printOn(std::ostream& s) const
{
	return s << "(" << typeid(*this).name() << ")";
}

std::shared_ptr<std::vector<Symsptr>> Symbolic::getTerms()
{
	assert(false);
	return std::make_shared<std::vector<Symsptr>>();
}

void MbD::Symbolic::addTerm(Symsptr trm)
{
	getTerms()->push_back(trm);
}

double Symbolic::getValue()
{
	assert(false);
	return 0.0;
}

void MbD::Symbolic::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	assert(false);
	return;
}

Symsptr MbD::Symbolic::clonesptr()
{
	//Return shallow copy of *this wrapped in shared_ptr
	assert(false);
	return std::make_shared<Symbolic>(*this);
}

void MbD::Symbolic::arguments(Symsptr args)
{
	assert(false);
}

Symsptr MbD::Symbolic::raisedTo(Symsptr x, Symsptr y)
{
	return std::make_shared<Power>(x, y);
}
