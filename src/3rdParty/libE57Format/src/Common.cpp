// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2020 PTC Inc.

#include "Common.h"

#include <random>

namespace e57
{

   std::string generateRandomGUID()
   {
      static constexpr const char UUID_CHARS[] = "0123456789ABCDEF";
      static std::random_device rd;
      static std::mt19937 gen( rd() );
      static std::uniform_int_distribution<> dis( 0, 15 /* number of chars in UUID_CHARS */ );

      std::string uuid( 38, ' ' );

      uuid[0] = '{';
      uuid[9] = '-';
      uuid[14] = '-';
      uuid[19] = '-';
      uuid[24] = '-';
      uuid[37] = '}';

      uuid[15] = '4';

      for ( int i = 1; i < 37; ++i )
      {
         if ( i != 9 && i != 14 && i != 19 && i != 24 && i != 15 )
         {
            uuid[i] = UUID_CHARS[dis( gen )];
         }
      }
      return uuid;
   }

} // end namespace e57
