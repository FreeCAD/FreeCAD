/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "Constant.h"
#include "System.h"
#include "Units.h"
#include "Polynomial.h"

using namespace MbD;

Constant::Constant()
{
}

Constant::Constant(double val) : Variable(val)
{
}

Symsptr MbD::Constant::differentiateWRT(Symsptr)
{
	return sptrConstant(0.0);
}

Symsptr MbD::Constant::integrateWRT(Symsptr var)
{
	if (value == 0.0) return clonesptr();
	auto slope = sptrConstant(value);
	auto intercept = sptrConstant(0.0);
	auto coeffs = std::make_shared<std::vector<Symsptr>>();
	coeffs->push_back(intercept);
	coeffs->push_back(slope);
	return std::make_shared<Polynomial>(var, coeffs);
}

Symsptr MbD::Constant::expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>>)
{
	return sptr;
}

bool Constant::isConstant()
{
	return true;
}

Symsptr MbD::Constant::clonesptr()
{
	return std::make_shared<Constant>(*this);
}

bool MbD::Constant::isZero()
{
	return value == 0.0;
}

bool MbD::Constant::isOne()
{
	return value == 1.0;
}

void MbD::Constant::createMbD(std::shared_ptr<System>, std::shared_ptr<Units>)
{
	return;
}

double MbD::Constant::getValue()
{
	return value;
}

double MbD::Constant::getValue(double)
{
	return value;
}

std::ostream& Constant::printOn(std::ostream& s) const
{
	return s << this->value;
}
