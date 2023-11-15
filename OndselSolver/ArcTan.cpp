/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ArcTan.h"

using namespace MbD;

MbD::ArcTan::ArcTan(Symsptr arg) : FunctionX(arg)
{
}

double MbD::ArcTan::getValue()
{
    return std::atan(xx->getValue());
}

Symsptr MbD::ArcTan::copyWith(Symsptr arg)
{
	return std::make_shared<ArcTan>(arg);
}

std::ostream& MbD::ArcTan::printOn(std::ostream& s) const
{
	s << "arctan(" << *xx << ")";
	return s;
}
