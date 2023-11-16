/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "Cosine.h"
#include "Sine.h"
#include "Negative.h"

using namespace MbD;

MbD::Cosine::Cosine(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Cosine::getValue()
{
    return std::cos(xx->getValue());
}

Symsptr MbD::Cosine::differentiateWRTx()
{
    return std::make_shared<Negative>(std::make_shared<Sine>(xx));
}

Symsptr MbD::Cosine::copyWith(Symsptr arg)
{
	return std::make_shared<Cosine>(arg);
}

std::ostream& MbD::Cosine::printOn(std::ostream& s) const
{
	s << "cos(" << *xx << ")";
	return s;
}
