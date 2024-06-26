/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <cmath>
#include <stdexcept>

#include "Numeric.h"

using namespace MbD;

double MbD::Numeric::arcTan0to2piYoverX(double y, double x)
{
	//"(y/x) arcTan in the range 0 to 2*pi."
	//"Double arcTan0to2piY: 1.0d overX: 1.0d."

	if (y >= 0) {
		//"First and second quadrants."
		return std::atan2(y, x);
	}
	else {
		//"Third and forth quadrants."
		return 2.0 * M_PI + std::atan2(y, x);
	}
}

bool MbD::Numeric::equaltol(double x, double xx, double tol)
{
	return std::abs(x - xx) < tol;
}
