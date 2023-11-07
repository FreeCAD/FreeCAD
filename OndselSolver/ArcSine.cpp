/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "ArcSine.h"

using namespace MbD;

MbD::ArcSine::ArcSine(Symsptr arg) : FunctionX(arg)
{
}

double MbD::ArcSine::getValue()
{
	return std::asin(xx->getValue());
}

Symsptr MbD::ArcSine::copyWith(Symsptr arg)
{
	return std::make_shared<ArcSine>(arg);
}

std::ostream& MbD::ArcSine::printOn(std::ostream& s) const
{
	s << "arcsin(" << *xx << ")";
	return s;
}
