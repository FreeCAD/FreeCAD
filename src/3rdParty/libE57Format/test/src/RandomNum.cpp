// libE57Format testing Copyright © 2022 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: MIT

#include <random>

#include "RandomNum.h"

namespace
{
   static std::random_device rd;

   // Use pseudo-random numbers unless we explicitly set the seed.
   static std::default_random_engine e( rd() );

   // Use a uniform distribution [0, 1].
   // For explanation of second param, see:
   //    https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
   static std::uniform_real_distribution<float> dis(
      0.0f, std::nextafter( 1.0f, std::numeric_limits<float>::max() ) );
}

namespace Random
{
   void seed( uint32_t inSeed )
   {
      e.seed( inSeed );
   }

   float num()
   {
      return dis( e );
   }
}
