/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "Sine.h"
#include "Cosine.h"

using namespace MbD;

MbD::Sine::Sine(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Sine::getValue()
{
    return std::sin(xx->getValue());
}

Symsptr MbD::Sine::differentiateWRTx()
{
    return std::make_shared<Cosine>(xx);
}

Symsptr MbD::Sine::copyWith(Symsptr arg)
{
	return std::make_shared<Sine>(arg);
}

std::ostream& MbD::Sine::printOn(std::ostream& s) const
{
	s << "sin(" << *xx << ")";
	return s;
}
