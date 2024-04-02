/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <algorithm>

#include "PiecewiseFunction.h"

using namespace MbD;

MbD::PiecewiseFunction::PiecewiseFunction()
{
	noop();
}

MbD::PiecewiseFunction::PiecewiseFunction(Symsptr var, std::shared_ptr<std::vector<Symsptr>> funcs, std::shared_ptr<std::vector<Symsptr>> trans)
{
	assert(var);
	assert(functions->empty());
	xx = var;
	functions->clear();
	functions->insert(functions->end(), funcs->begin(), funcs->end());
	transitions->clear();
	transitions->insert(transitions->end(), trans->begin(), trans->end());
}

Symsptr MbD::PiecewiseFunction::expandUntil(Symsptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	auto expansions = std::make_shared<std::vector<Symsptr>>();
	std::transform(functions->begin(),
		functions->end(),
		std::back_inserter(*expansions),
		[&](auto& func) { return func->expandUntil(func, set); }
	);
	return std::make_shared<PiecewiseFunction>(xx, expansions, transitions);
}

Symsptr MbD::PiecewiseFunction::simplifyUntil(Symsptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	auto simplifications = std::make_shared<std::vector<Symsptr>>();
	std::transform(functions->begin(),
		functions->end(),
		std::back_inserter(*simplifications),
		[&](auto& func) { return func->simplifyUntil(func, set); }
	);
	return std::make_shared<PiecewiseFunction>(xx, simplifications, transitions);
}

Symsptr MbD::PiecewiseFunction::differentiateWRTx()
{
	auto derivatives = std::make_shared<std::vector<Symsptr>>();
	std::transform(functions->begin(),
		functions->end(),
		std::back_inserter(*derivatives),
		[&](auto& func) { return func->differentiateWRT(xx); }
	);
	return std::make_shared<PiecewiseFunction>(xx, derivatives, transitions);
}

Symsptr MbD::PiecewiseFunction::integrateWRT(Symsptr var)
{
	assert(xx == var);
	auto integrals = std::make_shared<std::vector<Symsptr>>();
	std::transform(functions->begin(),
		functions->end(),
		std::back_inserter(*integrals),
		[var](auto& func) { return func->integrateWRT(var); }
	);
	for (size_t i = 0; i < transitions->size(); i++)
	{
		auto x = transitions->at(i)->getValue();
		auto fi = integrals->at(i)->getValue(x);
		auto fi1 = integrals->at(i + 1)->getValue(x);
		auto integConstant = fi - fi1;
		integrals->at(i + 1)->setIntegrationConstant(integConstant);
		noop();
	}
	return std::make_shared<PiecewiseFunction>(var, integrals, transitions);
}

double MbD::PiecewiseFunction::getValue()
{
	auto xval = xx->getValue();
	for (size_t i = 0; i < transitions->size(); i++)
	{
		if (xval < transitions->at(i)->getValue()) {
			return functions->at(i)->getValue();
		}
	}
	return functions->back()->getValue();
}

void MbD::PiecewiseFunction::arguments(Symsptr args)
{
	auto arguments = args->getTerms();
	xx = arguments->at(0);
	functions = arguments->at(1)->getTerms();
	transitions = arguments->at(2)->getTerms();
}

std::ostream& MbD::PiecewiseFunction::printOn(std::ostream& s) const
{
	s << "PiecewiseFunction(" << *xx << ", " << std::endl;
	s << "functions{" << std::endl;
	s << *functions->at(0) << std::endl;
	for (size_t i = 1; i < functions->size(); i++)
	{
		s << *functions->at(i) << std::endl;
	}
	s << "}, " << std::endl;
	s << "transitions{" << std::endl;
	s << *transitions->at(0) << std::endl;
	for (size_t i = 1; i < transitions->size(); i++)
	{
		s << *transitions->at(i) << std::endl;
	}
	s << "})" << std::endl;
	return s;
}
