/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "Abs.h"

using namespace MbD;

MbD::Abs::Abs(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Abs::getValue()
{
    return std::abs(xx->getValue());
}

std::ostream& MbD::Abs::printOn(std::ostream& s) const
{
	s << "abs(" << xx << ")";
	return s;
}
