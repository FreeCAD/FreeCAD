/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <cmath>
#include <corecrt_math_defines.h>
#include <stdexcept>

#include "Numeric.h"

using namespace MbD;

double MbD::Numeric::arcTan0to2piYoverX(double y, double x)
{
	//"(y/x) arcTan in the range 0 to 2*pi."
	//"Double arcTan0to2piY: 1.0d overX: 1.0d."

	if (x > 0.0) {
		if (y >= 0) {
			//"First quadrant."
			return std::atan2(y, x);
		}
		else {
			//"Forth quadrant."
			return 2.0 * M_PI + std::atan2(y, x);
		}
	}
	else {
		if (x < 0.0) {
			//"Second and Third quadrants."
			return	M_PI + std::atan2(y, x);
		}
		else {
			//"x = 0"
			if (y > 0.0) {
				return M_PI / 2.0;
			}
			else {
				if (y < 0.0) {
					return 1.5 * M_PI;
				}
				else {
					throw std::invalid_argument("atan2(0, 0) is not defined.");
				}
			}
		}
	}
}

bool MbD::Numeric::equaltol(double x, double xx, double tol)
{
	return std::abs(x - xx) < tol;
}
