// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2020 PTC Inc.

// for M_PI. This needs to be first, otherwise we might already include math header
// without M_PI and we would get nothing because of the header guards.
#define _USE_MATH_DEFINES
#include <cmath>

#include "E57SimpleData.h"

namespace e57
{

   // To avoid exposing M_PI, we define the constructor here.
   SphericalBounds::SphericalBounds()
   {
      rangeMinimum = 0.;
      rangeMaximum = E57_DOUBLE_MAX;
      azimuthStart = -M_PI;
      azimuthEnd = M_PI;
      elevationMinimum = -M_PI / 2.;
      elevationMaximum = M_PI / 2.;
   }

} // end namespace e57
