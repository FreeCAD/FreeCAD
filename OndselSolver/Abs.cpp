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
	//Do nothing.
}

double MbD::Abs::getValue()
{
    return std::abs(xx->getValue());
}

Symsptr MbD::Abs::copyWith(Symsptr arg)
{
	return std::make_shared<Abs>(arg);
}

std::ostream& MbD::Abs::printOn(std::ostream& s) const
{
	s << "abs(" << *xx << ")";
	return s;
}
