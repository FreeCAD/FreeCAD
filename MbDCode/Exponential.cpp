/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "Exponential.h"

using namespace MbD;

MbD::Exponential::Exponential(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Exponential::getValue()
{
    return std::log(xx->getValue());
}

std::ostream& MbD::Exponential::printOn(std::ostream& s) const
{
	s << "exp(" << xx << ")";
	return s;
}
