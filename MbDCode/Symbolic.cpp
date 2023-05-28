#include <memory>
#include <unordered_set>
#include <assert.h>

#include "Symbolic.h"
#include "Constant.h"

using namespace MbD;

Symbolic::Symbolic()
{
}

void Symbolic::initialize()
{
}

std::shared_ptr<Symbolic> Symbolic::differentiateWRT(std::shared_ptr<Symbolic> var)
{
	assert(false);
	return std::shared_ptr<Symbolic>();
}

std::shared_ptr<Symbolic> Symbolic::simplified()
{
	//Debug 
	auto set = nullptr;
	//Debug auto set = std::make_shared<std::unordered_set<Symbolic>>();
	auto answer = expandUntil(set);
	auto set1 = nullptr; //std::make_shared<std::unordered_set<Symbolic>>();
	return answer->simplifyUntil(set1);
}

std::shared_ptr<Symbolic> Symbolic::expandUntil(std::shared_ptr<std::unordered_set<Symbolic>> set)
{
	assert(false);
	return std::make_shared<Constant>(0.0);
}

std::shared_ptr<Symbolic> Symbolic::simplifyUntil(std::shared_ptr<std::unordered_set<Symbolic>> set)
{
	assert(false);
	return std::make_shared<Constant>(0.0);
}

double MbD::Symbolic::getValue()
{
	assert(false);
	return 0.0;
}
