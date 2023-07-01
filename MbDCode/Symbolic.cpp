#include <memory>
#include <unordered_set>
#include <assert.h>

#include "Symbolic.h"
#include "Constant.h"
#include "Product.h"
#include "Sum.h"

using namespace MbD;

Symbolic::Symbolic()
{
}

void Symbolic::initialize()
{
}

Symsptr Symbolic::differentiateWRT(Symsptr sptr, Symsptr var)
{
	assert(false);
	return Symsptr();
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

Symsptr Symbolic::expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	return sptr;
}

Symsptr Symbolic::simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	return sptr;
}

Symsptr Symbolic::timesSum(Symsptr sptr, Symsptr sum)
{
	auto product = std::make_shared<Product>(sptr);
	return product->timesSum(product, sum);
}

Symsptr Symbolic::timesProduct(Symsptr sptr, Symsptr product)
{
	auto answer = std::make_shared<Product>(product->getTerms());
	auto& answerTerms = answer->terms;
	answerTerms->insert(answerTerms->begin(), sptr);
	return answer;
}

Symsptr Symbolic::timesFunction(Symsptr sptr, Symsptr function)
{
	auto answer = std::make_shared<Product>(sptr, function);
	return answer;
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
	return std::make_shared<std::vector<Symsptr>>();
}

double Symbolic::getValue()
{
	assert(false);
	return 0.0;
}
