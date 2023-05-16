#include <memory>
#include <unordered_set>
#include <assert.h>

#include "Symbolic.h"
#include "Constant.h"

using namespace MbD;

Symbolic::Symbolic()
{
	initialize();
}

void Symbolic::initialize()
{
}

std::shared_ptr<Symbolic> MbD::Symbolic::differentiateWRT(std::shared_ptr<Symbolic> var)
{
	return std::shared_ptr<Symbolic>();
}

std::shared_ptr<Symbolic> MbD::Symbolic::simplified()
{
	//Debug 
	auto set = nullptr;
	//Debug auto set = std::make_shared<std::unordered_set<Symbolic>>();
	auto answer = expandUntil(set);
	auto set1 = nullptr; //std::make_shared<std::unordered_set<Symbolic>>();
	return answer->simplifyUntil(set1);
}

std::shared_ptr<Symbolic> MbD::Symbolic::expandUntil(std::shared_ptr<std::unordered_set<Symbolic>> set)
{
	return std::make_shared<Constant>(0.0);
}

std::shared_ptr<Symbolic> MbD::Symbolic::simplifyUntil(std::shared_ptr<std::unordered_set<Symbolic>> set)
{
	return std::make_shared<Constant>(0.0);
}
