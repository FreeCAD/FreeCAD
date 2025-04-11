/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <algorithm>

#include "FunctionX.h"
#include "Constant.h"
#include "Sum.h"
#include "Arguments.h"

using namespace MbD;

MbD::FunctionX::FunctionX(Symsptr arg) : xx(arg)
{
}

void MbD::FunctionX::arguments(Symsptr args)
{
	auto arguments = std::static_pointer_cast<Arguments>(args);
	assert(arguments->terms->size() == 1);
	xx = arguments->terms->front();
}

Symsptr MbD::FunctionX::copyWith(Symsptr self)
{
	return self;
}

Symsptr MbD::FunctionX::expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	auto itr = std::find_if(set->begin(), set->end(), [sptr](Symsptr sym) {return sptr.get() == sym.get(); });
	if (itr != set->end()) return sptr;
	auto newx = xx->expandUntil(xx, set);
	auto copy = copyWith(newx);
	if (newx->isConstant()) {
		return sptrConstant(copy->getValue());
	}
	else {
		return copy;
	}
}

Symsptr MbD::FunctionX::simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	auto itr = std::find_if(set->begin(), set->end(), [sptr](Symsptr sym) {return sptr.get() == sym.get(); });
	if (itr != set->end()) return sptr;
	auto newx = xx->simplifyUntil(xx, set);
	auto copy = copyWith(newx);
	if (newx->isConstant()) {
		return sptrConstant(copy->getValue());
	}
	else {
		return copy;
	}
}

Symsptr MbD::FunctionX::differentiateWRT(Symsptr var)
{
	if (this == var.get()) return sptrConstant(1.0);
	auto dfdx = differentiateWRTx();
	auto dxdvar = xx->differentiateWRT(var);
	return Symbolic::times(dfdx, dxdvar);
}

Symsptr MbD::FunctionX::differentiateWRTx()
{
	assert(false);
	return Symsptr();
}

void MbD::FunctionX::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	xx->createMbD(mbdSys, mbdUnits);
}

double MbD::FunctionX::getValue()
{
	assert(false);
	return 0.0;
}

double MbD::FunctionX::getValue(double arg)
{
	double answer;
	if (xx->isVariable()) {
		auto oldVal = xx->getValue();
		xx->setValue(arg);
		answer = getValue();
		xx->setValue(oldVal);
	}
	else {
		assert(false);
	}
	return answer;
}

bool MbD::FunctionX::isConstant()
{
	return xx->isConstant();
}
