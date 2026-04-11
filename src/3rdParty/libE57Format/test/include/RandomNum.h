#pragma once
// libE57Format testing Copyright © 2022 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: BSL-1.0

#include <cstdint>

namespace Random
{
   // Seed our pseudo-random number generator for reproducibility.
   void seed( uint32_t inSeed );

   // Get a pseudo-random number.
   float num();
}
