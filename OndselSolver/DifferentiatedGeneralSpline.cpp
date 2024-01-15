/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DifferentiatedGeneralSpline.h"
#include "GeneralSpline.h"

using namespace MbD;

MbD::DifferentiatedGeneralSpline::DifferentiatedGeneralSpline(Symsptr arg, Symsptr spline, size_t derivOrder) : AnyGeneralSpline(arg), generalSpline(spline), derivativeOrder(derivOrder)
{
}

double MbD::DifferentiatedGeneralSpline::getValue()
{
	return std::static_pointer_cast<GeneralSpline>(generalSpline)->derivativeAt(derivativeOrder, xx->getValue());
}

Symsptr MbD::DifferentiatedGeneralSpline::differentiateWRTx()
{
	auto arg = std::static_pointer_cast<GeneralSpline>(generalSpline)->xx;
	auto deriv = std::make_shared<DifferentiatedGeneralSpline>(arg, generalSpline, derivativeOrder + 1);
	return deriv;
}

Symsptr MbD::DifferentiatedGeneralSpline::clonesptr()
{
	return std::make_shared<DifferentiatedGeneralSpline>(*this);
}

std::ostream& MbD::DifferentiatedGeneralSpline::printOn(std::ostream& s) const
{
	s << "deriv(" << *generalSpline << ", " << derivativeOrder << ")";
	return s;
}
