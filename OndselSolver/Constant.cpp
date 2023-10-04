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

using namespace MbD;

Constant::Constant()
{
}

Constant::Constant(double val) : Variable(val)
{
}

Symsptr MbD::Constant::differentiateWRT(Symsptr var)
{
	return std::make_shared<Constant>(0.0);
}

bool Constant::isConstant()
{
	return true;
}

Symsptr MbD::Constant::expandUntil(std::shared_ptr<std::unordered_set<Symsptr>> set)
{
	return clonesptr();
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

void MbD::Constant::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	return;
}

double MbD::Constant::getValue()
{
	return value;
}

std::ostream& Constant::printOn(std::ostream& s) const
{
	return s << this->value;
}
